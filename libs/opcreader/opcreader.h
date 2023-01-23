#ifndef OPCREADER_H
#define OPCREADER_H

#include <array>
#include <list>
#include <map>
#include <string>
#include <string_view>
#include <atomic>

#include <spdlog/spdlog.h>

#include <nlohmann/json.hpp>

#include <OPCClient.h>
#include <OPCGroup.h>
#include <OPCHost.h>
#include <OPCItem.h>
#include <OPCServer.h>
#include <opcda.h>

enum struct opc_data_types { UNKNOWN, STRING, FLOAT, BYTE, WORD, INT };

struct opc_data_point {
  std::string name;
  std::string label;
  opc_data_types dataType;
};

class opc_reader {
 public:
  explicit opc_reader(std::string t_init_file_name);
  bool init();
  void query_server();
  void stop_query();

 protected:
  bool read_ini_file(std::string init_file_name);

  // bool connect_to_server();

  bool read_opc_item(nlohmann::json const& obj, opc_data_point& dp);

  opc_data_types match_opc_data_types(std::string sdt);

 private:
  bool init_ok{false};

  std::string init_file_name;

  std::string host_name;
  std::string opc_server_name;

  unsigned long query_interval_ms{2000};
  unsigned long retry_interval_ms{2000};

  bool report_response_time;

  std::vector<opc_data_point> vec_opc_data;

  std::atomic<bool> stop_querry_loop{false};
};

#endif  // OPCREADER_H
