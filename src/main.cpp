#include <vector>
#include <map>
#include <functional>
#include <filesystem>
#include <iostream>
#include <chrono>
#include <sys/types.h>
#include <sys/stat.h>
#include <tuple>
#include <sstream>

#include "yaap.hpp"

namespace fs = std::filesystem;
namespace timer = std::chrono;

template<typename Accessor>
const time_t get_file_date( const fs::path& f, Accessor access ){
  struct stat finfo; 
  if( stat(f.c_str(), &finfo) == 0){
    const time_t file_time = access(finfo);
    return file_time;
  }
  else{
    throw std::runtime_error("Invalide file");
  }
}

bool ends_with(std::string const & s, std::string const & end)
{
    if (end.size() > s.size()) return false;
    return std::equal(end.rbegin(), end.rend(), s.rbegin());
}

time_t make_time_limit( std::string d){
    std::vector<std::string> tokens; 
    size_t pos;
    std::string token;
    while( (pos=d.find(":")) != std::string::npos  ){
      tokens.push_back( d.substr(0, pos) );
      d.erase(0, pos + 1);
    }
    tokens.push_back(d);

    std::array<int,2> date_tokens = {0,0};

    for( auto t: tokens){
      if( ends_with(t, "y") ){
        date_tokens[0] = atoi(t.substr(0, t.length()-1).c_str());
      }
      else if( ends_with(t, "m") ){
        date_tokens[1] = atoi(t.substr(0, t.length()-1).c_str());
      }
      else {
        throw std::runtime_error("Invalid date formating");
      }
    }

    time_t now = time(0);
    tm* tnow = localtime(&now);
    tm tmp = tm();
    tmp.tm_year = tnow->tm_year - std::get<0>(date_tokens);
    if(tnow->tm_mon < std::get<1>(date_tokens) ){
      tmp.tm_year -= (1 + int(tnow->tm_mon-std::get<1>(date_tokens))/int(12) );
      tmp.tm_mon = 12+ int(tnow->tm_mon-std::get<1>(date_tokens))%int(12); 
    }
    else{
      tmp.tm_mon = tnow->tm_mon - std::get<1>(date_tokens);
    }

    tmp.tm_mday = tnow->tm_mday;    
  std::cout << "Time limit: " << asctime(&tmp) << std::endl;
  return mktime(&tmp);
}



int main(const int argc, char** argv) {

  std::cout << "==== START FILE SYSTEM PROFILING ====" << std::endl;
  std::cout << " " << std::endl;


  auto timer_start = std::chrono::high_resolution_clock::now();

  /* Define accessors function for time atime, ctime, ... */ 
  std::map<std::string, std::function<time_t (const struct stat&)> > accessors;
  accessors["atime"] = [](const auto& finfo){ return finfo.st_atime; };
  accessors["ctime"] = [](const auto& finfo){ return finfo.st_ctime; };

  /* Define post actions functions */ 
  std::map<std::string, std::function<void (const fs::path&, std::stringstream& )> > post_actions; 
  post_actions["none"] = [](const fs::path& f, std::stringstream& ss) -> void {
      
  };
  post_actions["rpath"] = [](const fs::path& f, std::stringstream& ss) -> void  {
      ss << fs::relative(f) << " ";
  };
  post_actions["apath"] = [](const fs::path& f, std::stringstream& ss) -> void {
      ss << f << " ";
  };


  ulong tot_size=0;
  post_actions["size"] = [&tot_size](const fs::path& f, std::stringstream& ss) -> void {
      auto sz = fs::file_size(f);
      tot_size += sz;
      if( sz >= 1.e8 ){ // Go
        ss << sz / 1.e9 <<  "G ";
      }
      else if( sz >= 1.e5 ){ // Mo 
        ss << sz / 1.e6 << " M ";
      }
      else if( sz >= 1.e2 ){ // Ko 
        ss << sz / 1.e3 << " K ";
      }
      else{
        ss << sz << " o ";
      }
  };


  /* Argument parser  */  
  yaap::ArgumentParser* args = yaap::ArgumentParser::Instance();
  args->addArg<bool>("-h", "--help", yaap::ARG::OPTIONAL, false, "Print help message");
  args->addArg<std::string>("-t", "--time", yaap::ARG::OPTIONAL, "atime", "Time to consider for profiling atime or ctime");
  args->addArg<std::string>("-o", "--older", yaap::ARG::OPTIONAL, "1y", "Time limit format Ny:Mm"); // 1y or 6m or 1y6m
  args->addArg<bool>("-e", "--error", yaap::ARG::OPTIONAL, false, "Print error message");
  args->addArg<std::vector<std::string> >("-p", "--post", yaap::ARG::OPTIONAL, {"rpath"}, "Post action to perform"); // -p path or -p path, size
  args->addInput<std::vector<std::string> >("directories", yaap::ARG::REQUIRED, "Directories to explore");

  try{
    args->parse(argc, argv);
  }
  catch(...){ 
    args->usage();
    return 1; 
  }
  
  if( args->argVal<bool>("-h") ){
    args->usage();
    return 0;
  }

  auto dirs = args->inputVal<std::vector<std::string> >( "directories" );

  auto date = args->argVal<std::string>("-o");
  
  time_t time_limit = make_time_limit( date ); 
  std::cout << " Time limit: " << asctime(localtime(&time_limit)) << std::endl;
  
  std::vector<fs::path> path2check;
  for(auto d: dirs){
    auto p = fs::path(d);
    if( fs::exists(p) ){
      path2check.push_back(p);
    } 
    else {
      throw std::runtime_error("Invalid directory " + d);
    }
  }


  fs::path current_dir = dirs[0];
  unsigned int count_checked=0;
  unsigned int count_error=0;
  
  std::vector<std::tuple<fs::path, time_t> > file2keep;
  std::vector<fs::path> fileInError;



  for( auto &dir: dirs){
    for (auto &file : fs::recursive_directory_iterator(dir)) {
      if( file.is_directory() ){
        continue;
      }
      try {
        //auto ftime = fs::last_write_time(file);
        const auto t = get_file_date(file, accessors[args->argVal<std::string>("-t")]);
        if( difftime(t, time_limit) < 0. ){
          file2keep.push_back( std::tuple(file, t) );
        }
        count_checked++;
      } catch( std::exception& e ) {
        count_error++;
        fileInError.push_back( file );
        //std::cout << e.what() << std::endl;
        continue; 
      }
    }
  }

  for( auto x: file2keep){
    auto f = std::get<0>(x);
    std::stringstream stream;
    stream << "";
    for( auto post: args->argVal<std::vector<std::string> >("-p") ){
      post_actions[post](f, stream);
    } 
    std::cout << stream.str() << std::endl;
  }

  if( args->argVal<bool>("-e") && fileInError.size() != 0){
    std::cout << "---------- File system error path ----------- " << std::endl;
    for( auto f: fileInError){
      std::cout << f << std::endl;
    } 
  }

  std::cout << "=========== END FILE SYSTEM PROFILING ========== " << std::endl;
  std::cout << " #File checked = " << file2keep.size() << std::endl;
  std::cout << " #File error = " << fileInError.size() << std::endl;
  if( fileInError.size() != 0){
    std::cout << " To print the list of file system error use -e, --error option " << std::endl;
  }
  auto timer_stop = std::chrono::high_resolution_clock::now();      
  if( tot_size > 0. ){
    std::cout << " Cumulated size: " << tot_size / 1.e9 << " G" << std::endl; 
  }


  std::cout << " Time (ms): " << std::chrono::duration_cast<std::chrono::milliseconds>(timer_stop - timer_start).count() << std::endl;
  std::cout << "================================================ " << std::endl;
  return 0;
}
