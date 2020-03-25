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

std::string ReplaceAll(std::string &str, const char& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, 1, to);
        start_pos += to.length();
    }
    return str;
}

const std::string escapeJSON(const std::string &raw) {
    std::string escaped = raw;
    ReplaceAll(escaped, '\\', "\\\\");
    ReplaceAll(escaped, '\n', "\\n");
    ReplaceAll(escaped, '\f', "\\f");
    ReplaceAll(escaped, '\r', "\\r");
    ReplaceAll(escaped, '\t', "\\t");
    ReplaceAll(escaped, '\b', "\\b");
    ReplaceAll(escaped, '"', "\\\"");

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

std::string convertLine2JSONL(const std::vector<std::optional<std::string>> &fields, 
    const opt_str_list header, const bool auto_convert){
    std::stringstream tmp_json;

    int i = 0;

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
    //return "test";
}

/**
 * nice hack from: 
 * https://bytes.com/topic/c/answers/841283-how-make-non-blocking-call-cin
 */
void set_stdin_block(const bool block){
    const int fd = fileno(stdin);
    termios flags;
    if (tcgetattr(fd,&flags)<0) {
        std::cerr << "not able to set flags" << std::endl;
    }
    // set raw (unset canonical modes)
    flags.c_lflag &= ~ICANON; 
    // i.e. min 1 char for blocking, 0 chars for non-blocking
    flags.c_cc[VMIN] = block; 
    // block if waiting for char
    flags.c_cc[VTIME] = 0; 

    if (tcsetattr(fd,TCSANOW,&flags)<0) {
        std::cerr << "not able to set flags" << std::endl;
    }
}

void parseTSV(std::istream &in, std::ostream &out,
    const opt_str_list &header, const bool auto_convert){
    std::stringstream tmp_field;
    std::vector<std::optional<std::string>> fields;

    char c;
    bool escaped = false, null_val = false;

    while( in.get(c)) {
        if (c == '\\') {
            escaped = true;
            continue;
        }

        if (!escaped){
            switch (c) {
                case '\n':
                    // convert & clear buffer
                    out << convertLine2JSONL(fields, header, auto_convert);

                    tmp_field.clear();
                    tmp_field.str(std::string());

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
                    break;

                default:
                    tmp_field << c;
            }
        }

        // if there is a \N it is null
        else if ( (c == 'N') && escaped) {
            null_val = true;
        } else {
            tmp_field << c;
        }
        
        escaped = false;
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


int main(int argc, char * argv[]){
    int opt;
    std::vector<std::string> header;
    bool auto_convert = false;

    while ((opt = getopt(argc, argv, "hn:a")) != -1) {
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

        parseTSV(is, std::cout, std::optional(header), auto_convert);
    } else if (argc - optind  == 2) {
        // outputfile defined
        std::ifstream is(argv[optind]);
        std::ofstream os(argv[optind + 1]);

        parseTSV(is, os, std::optional(header), auto_convert);
    } else {
        parseTSV(std::cin, std::cout, std::optional(header), auto_convert);
    }
}