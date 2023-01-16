#ifndef OPCREADER_H
#define OPCREADER_H

#include <list>
#include <map>
#include <string>

#include <spdlog/spdlog.h>

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

 protected:
  bool read_ini_file(std::string init_file_name);

  bool connect_to_server();
  void query_server();

 private:
  bool init_ok{false};
  
  std::string init_file_name;

  std::string host_name;
  std::string opc_server_name;

  unsigned long query_interval_ms;
  unsigned long retry_interval_ms;

  bool report_response_time;

  std::vector<opc_data_point> vec_opc_data;
  std::map<COPCItem*, opc_data_point> map_opc_items;

  COPCHost* ptr_host{nullptr};
  COPCServer* ptr_opc_server{nullptr};
  COPCGroup* ptr_group{nullptr};

  std::vector<COPCItem*> vec_opc_items;
};

#endif  // OPCREADER_H
