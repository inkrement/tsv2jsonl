#include <istream>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <optional>
#include <unistd.h>
#include <regex>
#include <termios.h>

using opt_str_list = std::optional<std::vector<std::string>>;

static std::regex re_json_number("\\d*[.\\d*]");

// TODO: check how bool is encoded in mysql. likely int and not like this..
//static std::regex re_json_bool("true|false");

void printUsage(void){
    std::cerr << "tsv2jsonl [-h header_names] [-a] [in_file] [out_file]" << std::endl;

    std::cerr << std::endl << "\theader_names X1,X2,X3,.." << std::endl;
    std::cerr << "\t-a autodetect types" << std::endl;
}

const std::string escapeJSON(const std::string &raw) {
    std::string escaped;
    escaped.reserve(raw.length());

    for (std::string::size_type i = 0; i < raw.length(); ++i) {
        switch (raw[i]) {
            case '"':
                escaped += "\\\"";
                break;
            case '/':
                escaped += "\\/";
                break;
            case '\b':
                escaped += "\\b";
                break;
            case '\f':
                escaped += "\\f";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            case '\\':
                escaped += "\\\\";
                break;
            default:
                escaped += raw[i];
                break;
        }
    }

    return escaped;
}


std::string autoconvert(const std::string &val){
    // detect numbers
    if (std::regex_match (val, re_json_number))
        return val;
    // detect bool
    //else if (std::regex_match (val, re_json_bool))
    //    return val;

    return "\"" + escapeJSON(val) + "\"";
}

std::string debugLine(const std::vector<std::optional<std::string>> &fields, 
    const opt_str_list header){
    std::stringstream tmp_json;

    uint8_t i = 0;

    for (std::optional<std::string> opt_f: fields){
        // print key
        tmp_json << "\t";

        if(header.has_value() && header.value().size() > i){
            tmp_json << header.value().at(i);
        } else {
            tmp_json << "X" << i + 1;
        }

        tmp_json << ": ";

        // print value
        if(opt_f.has_value()){
            // print as string
            tmp_json << "\"" << escapeJSON(opt_f.value()) << "\"" << std::endl;
        } else {
            tmp_json << "null" << std::endl;
        }

        i++;
    }

    return tmp_json.str();
}

std::string convertLine2JSONL(const std::vector<std::optional<std::string>> &fields, 
    const opt_str_list header, const bool auto_convert){
    std::stringstream tmp_json;

    uint8_t i = 0;

    // start json
    tmp_json << "{";

    for (std::optional<std::string> opt_f: fields){
        // print key
        tmp_json << "\"";

        if(header.has_value() && header.value().size() > i){
            tmp_json << header.value().at(i);
        } else {
            tmp_json << "X" << i + 1;
        }

        tmp_json << "\":";

        // print value
        if(opt_f.has_value()){
            if (auto_convert) {
                tmp_json << autoconvert(opt_f.value());
            } else {
                // print as string
                tmp_json << "\"" << escapeJSON(opt_f.value()) << "\"";
            }
            
        } else {
            tmp_json << "null";
        }

        i++;
        
        // print delimiter
        if (i < fields.size()) {
            tmp_json << ",";
        }
    }

    // end json
    tmp_json << "}" << std::endl;

    return tmp_json.str();
}

void parseTSV(std::istream &in, std::ostream &out,
    const opt_str_list &header, const bool auto_convert, u_int8_t threads = 1, uint16_t max_len = 2048){
    std::stringstream tmp_field;
    uint16_t tmp_len = 0;
    std::vector<std::optional<std::string>> fields;

    char c;
    bool escaped = false, null_val = false;
    uint8_t tsv_line = 1, parsed_line = 1;
    uint8_t reg_line_length = 0;
    std::string json_string;

    while( in.get(c)) {
        if (!escaped){
            switch (c) {
                case '\n':
                    // close last field
                    if (null_val){
                        fields.push_back(std::nullopt);
                        null_val = false;
                    } else {
                        fields.push_back(tmp_field.str());
                    }

                    json_string = convertLine2JSONL(fields, header, auto_convert);
                    
                    if (parsed_line == 1){
                        reg_line_length = fields.size();

                        out << json_string;
                    } else {
                        if (fields.size() != reg_line_length) {
                            out << json_string;

                            //DEBUG

                            std::cerr << std::endl << "WARN: TSV line " << unsigned(tsv_line) << " has a different length than first one." << std::endl;
                            std::cerr << std::endl << debugLine(fields, header) << std::endl;
                        } else {
                            out << json_string;
                        }
                    }
                    
                    parsed_line++;
                    tsv_line++;

                    // clear buffers
                    tmp_field.clear();
                    tmp_field.str(std::string());
                    tmp_len = 0;

                    fields.clear();
                    break;
                
                // if there is a tab (unescaped) it is a new field
                case '\t':
                    if (null_val){
                        fields.push_back(std::nullopt);
                        null_val = false;
                    } else {
                        fields.push_back(tmp_field.str());
                    }
                    
                    tmp_field.clear();
                    tmp_field.str(std::string());
                    tmp_len = 0;
                    break;

                case '\\':
                    escaped = true;
                    break;

                default:
                    if (tmp_len < max_len){
                        tmp_field << c;
                        tmp_len++;
                    }
            }
        } else {
            // escaped 
            
            // if there is a \N it is null
            if ( c == 'N' ) {
                null_val = true;
            } else if ( c == '\n' ) {
                tsv_line++;
                if (tmp_len < max_len){
                    tmp_field << c;
                    tmp_len++;
                }
            } else {
                if (tmp_len < max_len){
                    tmp_field << c;
                    tmp_len++;
                }
            }

            escaped = false;
        }
    }
}

std::vector<std::string> split(const std::string &str, const char &d) {
    std::string tmp; 
    std::stringstream ss(str);
    std::vector<std::string> result;

    while(getline(ss, tmp, ',')){
        result.push_back(tmp);
    }

    return result;
}


/**
 * nice hack from: 
 * https://bytes.com/topic/c/answers/841283-how-make-non-blocking-call-cin
 */
void set_stdin_block(const bool block){
    const int fd = fileno(stdin);
    termios flags;
    if (tcgetattr(fd,&flags)<0) {
        //std::cerr << "not able to set flags" << std::endl;
    }
    // set raw (unset canonical modes)
    flags.c_lflag &= ~ICANON; 
    // i.e. min 1 char for blocking, 0 chars for non-blocking
    flags.c_cc[VMIN] = block; 
    // block if waiting for char
    flags.c_cc[VTIME] = 0; 

    if (tcsetattr(fd,TCSANOW,&flags)<0) {
        //std::cerr << "not able to set flags" << std::endl;
    }
}


int main(int argc, char * argv[]){
    int opt;
    int threads = 4;
    std::vector<std::string> header;
    bool auto_convert = false;

    while ((opt = getopt(argc, argv, "hn:at:")) != -1) {
        switch (opt) {
        case 'h':
            printUsage();
            exit(EXIT_SUCCESS);
        case 'n':
            // process header
            header = split(optarg, ',');
            break;
        case 'a':
            auto_convert = true;
            break;
        case 't':
            threads = atoi(optarg);
            break;
        default:
            printUsage();
            exit(EXIT_FAILURE);
        }
    }

    // do not block on stdin
    set_stdin_block(false);

    if ( argc - optind  == 1 ) {
        // input file defined
        std::ifstream is(argv[optind]);

        parseTSV(is, std::cout, std::optional(header), auto_convert, threads);
    } else if (argc - optind  == 2) {
        // outputfile defined
        std::ifstream is(argv[optind]);
        std::ofstream os(argv[optind + 1]);

        parseTSV(is, os, std::optional(header), auto_convert, threads);
    } else {
        parseTSV(std::cin, std::cout, std::optional(header), auto_convert, threads);
    }
}