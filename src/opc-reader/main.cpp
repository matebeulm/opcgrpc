#include <filesystem>
#include <iostream>
#include <string>

#include <fmt/format.h>

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <lyra/lyra.hpp>

#include <opcreader.h>

void setup_logging_new(std::string const& logger_name,
                       spdlog::level::level_enum level = spdlog::level::info) {
  std::filesystem::path log_dir = std::filesystem::current_path();
  log_dir /= "logs";
  if (!std::filesystem::exists(log_dir)) {
    bool success = std::filesystem::create_directories(log_dir);
    if (!success) {
      fmt::print("could not create directory for log-file {}", log_dir.generic_string());
    }
  }

  try {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(level);

    auto log_file = fmt::format("logs/{}_log.txt", logger_name);
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
      log_file, 1048576 * 5, 3,
      true);  // last parameter is rotate_on_open
    file_sink->set_level(level);

    spdlog::sinks_init_list sink_list = {file_sink, console_sink};

    spdlog::logger logger(logger_name, sink_list.begin(), sink_list.end());

    // or you can even set multi_sink logger as default logger
    spdlog::set_default_logger(std::make_shared<spdlog::logger>(
      logger_name, spdlog::sinks_init_list({console_sink, file_sink})));

    spdlog::set_level(level);
  } catch (const spdlog::spdlog_ex& ex) {
    std::cout << "Log initialization failed: " << ex.what() << std::endl;
  }
}

int main(int argc, char** argv) {
  setup_logging_new("opc-reader");
  spdlog::info("{}", argv[0]);

  bool show_help{false};
  std::string config_file;
  bool debug_messages{false};

  auto cli = lyra::help(show_help) |
             lyra::opt(config_file, "config file")["-c"]["--config"]("config file").required() |
             lyra::opt(debug_messages)["-d"]["--debug"]("show debug messages");

  auto parse_result = cli.parse({argc, argv});
  if (!parse_result) {
    spdlog::error("error in command line: {}", parse_result.message());
    show_help = true;
  }

  if (show_help) {
    std::stringstream cli_out;
    cli_out << cli;
    spdlog::info("{}", cli_out.str());
    return EXIT_SUCCESS;
  }

  auto dbg_level = debug_messages ? spdlog::level::debug : spdlog::level::info;
  spdlog::set_level(dbg_level);

  opc_reader reader(config_file);
  auto success = reader.init();

  return EXIT_SUCCESS;
}