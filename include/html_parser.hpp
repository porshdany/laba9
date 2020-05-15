// Copyright 2020 Usman Turkaev
#pragma once
#include <cctype>
#include <fstream>
#include <string>
#include <vector>

namespace html {
namespace parser {
std::string bring_to_standart_view(const std::string& string) {
  std::string standart_view;
  for (const auto& ch : string) {
    if (!std::isspace(ch)) standart_view += ch;
  }
  return standart_view;
}

std::vector<std::string> find_links(const std::string& html_code) {
  std::vector<std::string> links;
  size_t last_pos = 0;
  while (html_code.find("<a", last_pos) != std::string::npos) {
    size_t open_tag_pos = html_code.find("<a", last_pos) + 2;
    size_t close_tag_pos = html_code.find("</a>", open_tag_pos);
    size_t ref_begin = html_code.find("href=\"", open_tag_pos) + 6;
    size_t ref_end = html_code.find("\"", ref_begin);
    std::string link = html_code.substr(ref_begin, ref_end - ref_begin);
    last_pos = close_tag_pos + 4;
    if (link.find("http") != std::string::npos &&
        link.find("www") == std::string::npos &&
        link.find("vk") == std::string::npos) {
      links.push_back(link);
    }
  }
  return links;
}

inline bool is_image(const std::string& url) {
  return url.find_last_of(".") != std::string::npos &&
         url.find_last_of(".") > url.find_last_of("/");
}

std::string divide_by_noscript(const std::string& contents) {
  std::string noscript;
  if (contents.find("<!DOCTYPE HTML>") != std::string::npos ||
      contents.find("<!DOCTYPE html>") != std::string::npos) {
    size_t last_pos = 0;
    while (contents.find("<script", last_pos) != std::string::npos) {
      size_t script_begin_pos = contents.find("<script", last_pos);
      noscript += contents.substr(last_pos, script_begin_pos - last_pos);
      last_pos = contents.find("</script>", last_pos) + 9;
    }
    noscript += contents.substr(last_pos, contents.size() - last_pos);
  }
  return noscript;
}

std::vector<std::string> find_images(const std::string& contents) {
  std::string html_code = divide_by_noscript(contents);
  std::vector<std::string> images;
  size_t last_pos = 0;
  while (html_code.find("<img", last_pos) != std::string::npos) {
    size_t open_tag_pos = html_code.find("<img", last_pos) + 4;
    size_t close_tag_pos = html_code.find("/>", open_tag_pos);
    size_t ref_begin = html_code.find("src=\"", open_tag_pos) + 5;
    size_t ref_end = html_code.find("\"", ref_begin);
    std::string image = html_code.substr(ref_begin, ref_end - ref_begin);
    last_pos = close_tag_pos + 2;
    if (image.find("http") != std::string::npos && is_image(image)) {
      images.push_back(bring_to_standart_view(image));
    }
  }
  return images;
}
}  // namespace parser

std::string read_html_file(std::string filename = "downloads/page.html") {
  std::ifstream file(filename);
  if (file.is_open()) {
    std::string html_code;
    while (!file.eof()) {
      std::string line;
      std::getline(file, line);
      html_code += line;
    }
    file.close();
    return html_code;
  } else {
    std::logic_error("File with path " + filename + " does not exist");
    return "";
  }
}
}  // namespace html
