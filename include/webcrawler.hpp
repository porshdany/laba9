// Copyright 2020 Usman Turkaev
#pragma once
#include <curl/curl.h>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/thread.hpp>
#include <cstdlib>
#include <cstring>
#include <directory_manager.hpp>
#include <html_parser.hpp>
#include <iostream>
#include <producer_consumer.hpp>
#include <string>

using namespace boost::program_options;
namespace fs = boost::filesystem;

namespace webcrawler {
size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream) {
  size_t written = fwrite(ptr, size, nmemb, stream);
  return written;
}

char* str_to_char(const std::string& str) {
  char* arr = new char[str.size() + 1];
  std::strcpy(arr, str.c_str());
  return arr;
}

std::string download_obj(const char url[], directory_manager& manager,
                         const std::string& mode) {
  std::string filename;
  if (mode == "html") {
    filename = manager.create_html_file();
  } else if (mode == "img") {
    std::string s_url = std::string(url);
    if (html::parser::is_image(s_url)) {
      filename = manager.create_image(s_url);
    }
  } else {
    return NULL;
  }

  char* pagefilename = str_to_char(filename);
  FILE* pagefile;
  CURL* curl_handle;
  curl_global_init(CURL_GLOBAL_ALL);
  /* init the curl session */
  curl_handle = curl_easy_init();
  /* set URL to get here */
  curl_easy_setopt(curl_handle, CURLOPT_URL, url);
  /* Switch on full protocol/debug output while testing */
  curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 0L);
  /* disable progress meter, set to 0L to enable it */
  curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
  /* send all data to this function  */
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);
  /* open the file */
  pagefile = fopen(pagefilename, "wb");
  if (pagefile) {
    /* write the page body to this file handle */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, pagefile);
    /* get it! */
    curl_easy_perform(curl_handle);
    /* close the header file */
    fclose(pagefile);
    manager.unlock();
  }
  /* cleanup curl stuff */
  curl_easy_cleanup(curl_handle);
  curl_global_cleanup();
  delete pagefilename;
  return filename;
}

std::vector<std::string> find_links(
    const std::string& url, directory_manager& manager,
    producer_consumer<std::string>& pages_paths) {
  char* curl = str_to_char(url);
  std::string filename = download_obj(curl, manager, "html");
  std::string html_code = html::read_html_file(filename);
  std::vector<std::string> vec = html::parser::find_links(html_code);
  delete curl;
  pages_paths.produce(filename);
  return vec;
}

void download_html_pages(producer_consumer<std::string>& pages_paths,
                         safe_deque<std::string>& urls,
                         std::vector<std::string>::iterator begin,
                         std::vector<std::string>::iterator end, size_t depth,
                         directory_manager& manager) {
  --depth;
  if (depth > 0) {
    for (std::vector<std::string>::iterator it = begin; it < end; ++it) {
      if (!urls.check_existance(*it)) {
        urls.push_back(*it);
        std::vector<std::string> links = find_links(*it, manager, pages_paths);
        download_html_pages(pages_paths, urls, links.begin(), links.end(),
                            depth, manager);
      }
    }
  }
}

void parse_files(producer_consumer<std::string>& pages_paths,
                 producer_consumer<std::string>& images_urls) {
  images_urls.start_producing();
  while (!pages_paths.empty() || pages_paths.is_producing()) {
    std::string filename;
    if (!pages_paths.empty()) {
      if (pages_paths.try_consume(filename)) {
        std::string html_code = html::read_html_file(filename);
        images_urls.produce(html::parser::find_images(html_code));
      } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
  images_urls.stop_producing();
}

void write(producer_consumer<std::string>& images_urls, const std::string& out,
           directory_manager& manager) {
  while (!images_urls.empty() || images_urls.is_producing()) {
    std::string url;
    if (!images_urls.empty()) {
      if (images_urls.try_consume(url)) {
        if (html::parser::is_image(url)) {
          char* curl = str_to_char(url);
          download_obj(curl, manager, "img");
        }
      } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
}

void get_options(variables_map& vm, int argc, char* argv[]) {
  options_description desc("Allowed options");
  desc.add_options()("help", "выводим вспомогательное сообщение")(
      "url", value<std::string>(), "адресс HTML страницы")(
      "depth", value<size_t>(), "глубина поиска по странице")(
      "network_threads", value<size_t>(),
      "количество потоков для скачивания страниц")(
      "parser_threads", value<size_t>(),
      "количество потоков для обработки страниц")(
      "output", value<std::string>(), "путь до выходного файла");
  store(parse_command_line(argc, argv, desc), vm);
  notify(vm);
}

void work(int argc, char* argv[]) {
  try {
    variables_map vm;
    get_options(vm, argc, argv);
    if (vm.count("url") && vm.count("depth") && vm.count("network_threads") &&
        vm.count("parser_threads") && vm.count("output")) {
      std::string url = vm["url"].as<std::string>();
      std::string output = vm["output"].as<std::string>();
      fs::path output_path(output);
      size_t depth = vm["depth"].as<size_t>();

      std::system(
          std::string("mkdir -p " + output_path.string() + "/pages/").c_str());
      std::system(
          std::string("mkdir -p " + output_path.string() + "/images/").c_str());

      producer_consumer<std::string> pages_paths;
      producer_consumer<std::string> images_urls;
      pages_paths.start_producing();
      images_urls.start_producing();

      safe_deque<std::string> urls;

      boost::thread_group download_th;
      boost::thread_group parse_th;
      boost::thread_group write_th;

      size_t network_th_cnt = vm["network_threads"].as<size_t>();
      size_t parser_th_cnt = vm["parser_threads"].as<size_t>();
      size_t write_th_cnt = 1;

      directory_manager pages(output_path.string());
      directory_manager images(output_path.string());

      std::vector<std::string> links = find_links(url, pages, pages_paths);

      for (size_t i = 0; i < network_th_cnt; ++i) {
        if (i != network_th_cnt - 1) {
          download_th.add_thread(new boost::thread(
              download_html_pages, std::ref(pages_paths), std::ref(urls),
              links.begin() + links.size() / network_th_cnt * i,
              links.begin() + links.size() / network_th_cnt * (i + 1), depth,
              std::ref(pages)));
        } else {
          download_th.add_thread(new boost::thread(
              download_html_pages, std::ref(pages_paths), std::ref(urls),
              links.begin() + links.size() / network_th_cnt * i, links.end(),
              depth, std::ref(pages)));
        }
      }
      for (size_t i = 0; i < parser_th_cnt; ++i) {
        parse_th.add_thread(new boost::thread(
            parse_files, std::ref(pages_paths), std::ref(images_urls)));
      }
      for (size_t i = 0; i < write_th_cnt; ++i) {
        write_th.add_thread(new boost::thread(
            write, std::ref(images_urls), std::ref(output), std::ref(images)));
      }
      download_th.join_all();
      pages_paths.stop_producing();
      parse_th.join_all();
      images_urls.stop_producing();
      write_th.join_all();
    } else {
      throw std::logic_error("Not all options were entered!");
    }
  } catch (const std::exception& e) {
    std::cerr << e.what();
  }
}
}  // namespace webcrawler
