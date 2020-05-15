// Copyright 2020 Usman Turkaev
#pragma once
#include <boost/filesystem.hpp>
#include <fstream>
#include <html_parser.hpp>
#include <mutex>
#include <string>

namespace fs = boost::filesystem;

class directory_manager {
 public:
  directory_manager() = default;

  directory_manager(const std::string& output) : output_(output) {}

  directory_manager(const directory_manager&) = delete;

  directory_manager(directory_manager&&) = delete;

  std::string create_html_file() {
    this->lock();
    fs::path path(this->output_ + "/pages/");
    std::string filename = "page";
    std::string extension = ".html";
    return path.string() + check_existance(path, filename, extension);
  }

  inline void unlock() { this->mutex_.unlock(); }

  std::string create_image(const std::string& url) {
    this->lock();
    if (html::parser::is_image(url)) {
      fs::path path(this->output_ + "/images/");
      std::string extension;
      std::string filename;
      size_t dot_pos = url.find_last_of(".");
      size_t slash_pos = url.find_last_of("/");
      extension = url.substr(dot_pos);
      filename = url.substr(slash_pos + 1, dot_pos - slash_pos - 1);
      return path.string() + check_existance(path, filename, extension);
    } else {
      return "";
    }
  }

 private:
  inline void lock() { this->mutex_.lock(); }

  static inline std::string get_name(const fs::directory_entry& obj) {
    return obj.path().stem().string();
  }

  static inline std::string get_ext(const fs::directory_entry& obj) {
    return obj.path().extension().string();
  }

  static std::string check_existance(const fs::path& path,
                                     const std::string& filename,
                                     const std::string& extension) {
    size_t number = 0;
    for (const fs::directory_entry& obj : fs::directory_iterator(path)) {
      if (fs::is_regular_file(obj.path())) {
        if (get_ext(obj) == extension &&
            get_name(obj).substr(0, filename.size()) == filename) {
          if (get_name(obj).substr(filename.size(), 1) == "_" &&
              std::stoi(get_name(obj).substr(filename.size() + 1))) {
            size_t index = std::stoi(get_name(obj).substr(filename.size() + 1));
            if (index >= number) {
              number = index + 1;
            }
          } else if (get_name(obj) == filename && number == 0) {
            number = 1;
          }
        }
      }
    }
    switch (number) {
      case 0:
        return filename + extension;
      default:
        return filename + "_" + std::to_string(number) + extension;
    }
  }

  std::mutex mutex_;

  std::string output_;
};
