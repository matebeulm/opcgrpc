#include "opcreader.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <variant>

#include <asio/ip/host_name.hpp>

#include <nlohmann/json.hpp>

opc_reader::opc_reader(std::string t_init_file_name) : init_file_name(t_init_file_name) {}

bool opc_reader::init() {
  if (!read_ini_file(init_file_name))
  {
    return false;
  }
  return true;
}

bool opc_reader::read_ini_file(std::string init_file_name) {
  auto path = std::filesystem::path(init_file_name);
  if (!path.is_absolute()) {
    auto wd = std::filesystem::current_path();
    path = wd / path;
  }

  if (!std::filesystem::exists(path)) {
    spdlog::error("opc_reader: ini file does not exists: {}", path.generic_string());
    return false;
  }

  std::ifstream ifs(path);

  nlohmann::json jall;
  try {
    jall = nlohmann::json::parse(ifs);
  } catch (const nlohmann::json::parse_error& e) {
    spdlog::error("opc_reader: error reading json file");
    spdlog::error("===============================");
    spdlog::error("message: {0}", e.what());
    spdlog::error("exception id:{0}", e.id);
    spdlog::error("byte position of error: {0}", e.byte);
    spdlog::error("===============================");
  }

  return true;
}

bool opc_reader::connect_to_server() {
  spdlog::info("opc_reader trying to establish connection to server {}", opc_server_name);
  COPCClient::init();

  // wir verbinden uns immer zu einem server der auf demselben rechner läuft
  std::string hostname = asio::ip::host_name();
  ptr_host = COPCClient::makeHost(hostname);

  // list available opc servers
  std::vector<std::string> vec_local_servers;
  try {
    ptr_host->getListOfDAServers(IID_CATID_OPCDAServer20, vec_local_servers);
  } catch (OPCException& ex) {
    spdlog::warn("opc_reader could not connect to OPC server: {}", ex.reasonString());
    return false;
  }

  if (vec_local_servers.empty()) {
    spdlog::error("opc_reader no opc DA servers available on {}", hostname);
    return false;
  }

  // gibt es unseren server
  if (std::ranges::find(vec_local_servers, opc_server_name) == vec_local_servers.end()) {
    return false;
  }

  // connect to opc server
  try {
    ptr_opc_server = ptr_host->connectDAServer(opc_server_name);
  } catch (OPCException& ex) {
    spdlog::warn("opc_reader: could not connect to OPC server {}", ex.reasonString());
    return false;
  }

  // Check status
  ServerStatus status;
  ptr_opc_server->getStatus(status);
  spdlog::info("{}: server state is {}", opc_server_name, status.dwServerState);
  if (status.dwServerState != OPCSERVERSTATE::OPC_STATUS_RUNNING) {
    spdlog::error("opc_reader: opc server state != RUNNING");
    return false;
  }

  // make group
  unsigned long refresh_rate;
  ptr_group = ptr_opc_server->makeGroup(
    "Group", true, static_cast<unsigned long>(query_interval_ms), refresh_rate, 0.0);
  if (refresh_rate != static_cast<unsigned long>(query_interval_ms)) {
    spdlog::warn("opc_reader: {} requested update rate was {} but got {}", opc_server_name,
                 query_interval_ms, refresh_rate);

    if (refresh_rate > query_interval_ms) {
      query_interval_ms = refresh_rate;
      spdlog::info("opc_reader: adjusting query interval time to {} milliseconds",
                   query_interval_ms);
    }
  }

  // add our items to group
  for (auto data_point : vec_opc_data) {
    try {
      COPCItem* new_item = ptr_group->addItem(data_point.name, true);
      vec_opc_items.push_back(new_item);
      map_opc_items[new_item] = data_point;
    } catch (OPCException& ex) {
      spdlog::warn("opc_reader could not add OPC item <<{}>> reason: {}", data_point.name,
                   ex.reasonString());
    }
  }

  if (vec_opc_data.size() != vec_opc_data.size()) {
    spdlog::warn("opc_reader only {} out of {} items created", vec_opc_data.size(),
                 vec_opc_data.size());
  } else {
    spdlog::info("opc_reader only {} out of {} items created", vec_opc_data.size(),
                 vec_opc_data.size());
  }

  if (vec_opc_items.empty()) {
    spdlog::error("opc_reader: non of the querry items is available on server!");
    spdlog::error("opc_reader: shutting down!");
    return false;
  }

  return true;
}

void opc_reader::query_server() {}

// OPCReader::OPCReader() {
//   readerConnected = false;
//   group = nullptr;
//   demoMode = false;
//   demoTestDataMode = false;
//   // timerQuery = new QTimer();  // connects erfolgen am ende von readConfig je nachdem ob demo
//   mode
//   // oder nicht timerReStart = new QTimer(); connect(timerReStart, &QTimer::timeout, this,
//   // &OPCReader::startOPC); connect(this, &OPCReader::startOPCInfo, this,
//   // &OPCReader::startQueryServer);
// }

// bool OPCReader::readConfig(std::string opcIniFileName) {
// QSettings set(opcIniFileName, QSettings::IniFormat);
// set.setIniCodec("UTF-8"); // damit umlaute auch richtig eingelesen werden

// QStringList lstChildGroups = set.childGroups();
// if (!lstChildGroups.contains("OPCReader"))
// {
//     QLOG_ERROR() << "cannot find group OPCReader in ini file >>" << opcIniFileName << "<<";
//     return false;
// }
// set.beginGroup("OPCReader");
// readerName = set.value("readerName", "OPCReader").toString();
// hostName = set.value("hostName", "localhost").toString();
// opcServerName = set.value("opcServerName", "").toString();
// errorOnHostNotFound = set.value("errorOnHostNotFound", true).toBool();
// queryIntervalMS = set.value("queryIntervalMS", 5000).toInt();

// retryIntervalMS = set.value("retryIntervalMS", 6000).toInt();
// timerReStart->setInterval(retryIntervalMS);

// demoMode = set.value("demoMode", false).toBool();

// int nItems = set.beginReadArray("opcItems");
// for (int i=0; i<nItems; i++) {
//     set.setArrayIndex(i);
//     QString itemName = set.value("name", "").toString();
//     if (itemName.isEmpty()) {
//         continue;
//     }
//     QString itemLabel = set.value("label", itemName).toString();
//     QString itemType = set.value("type", "").toString();

//     // erst mal den typ checken
//     OPCDataType itemDataType = OPCDataType::UNKNOWN;
//     if (itemType.compare("string", Qt::CaseInsensitive) == 0) {
//         itemDataType = OPCDataType::STRING;
//     }

//     if (itemType.compare("float", Qt::CaseInsensitive) == 0) {
//         itemDataType = OPCDataType::FLOAT;
//     }

//     if (itemType.compare("int", Qt::CaseInsensitive) == 0) {
//         itemDataType = OPCDataType::INT;
//     }

//     if (itemType.compare("word", Qt::CaseInsensitive) == 0) {
//         itemDataType = OPCDataType::WORD;
//     }

//     if (itemType.compare("byte", Qt::CaseInsensitive) == 0) {
//         itemDataType = OPCDataType::BYTE;
//     }

//     if (itemType.compare("unknown", Qt::CaseInsensitive) == 0) {
//         // dann weiter zum nächsten eintrag
//         continue;
//     }

//     OPCItem item;
//     item.name = itemName;
//     item.label = itemLabel;
//     item.dataType = itemDataType;
//     lstOpcItems.append(item);
// }

// set.endArray();

// auto filePathTraceOPC = set.value("filePathTracePOC", "data_trace_opc_log").toString();
// QString folderName = QDir::currentPath() + "/" + filePathTraceOPC;
// QDir exportDir(folderName);
// if (!exportDir.exists())
// {
//     exportDir.mkpath(exportDir.absolutePath());
// }
// filePathTraceOPC = exportDir.absolutePath();
// std::string filename = filePathTraceOPC.toStdString();
// auto logger_name = "/trace_opc.log";
// filename.append(logger_name);
// // Set the default logger to file logger
// int logSizeMB = set.value("logSizeMB", 50).toInt();
// int logSizeFiles = set.value("logSizeFiles", 3).toInt();
// auto max_size = 1048576 * logSizeMB;
// auto max_files = logSizeFiles;
// _logger_opc = spdlog::rotating_logger_st("trace_wl_logger", filename, max_size, max_files);
// _logger_opc->set_pattern("%v"); // only the message itself, as influx will not handle it
// otherwise _logger_opc->set_level(spdlog::level::info);
// _logger_opc->flush_on(spdlog::level::info);

// set.endGroup();

// timerQuery->setInterval(queryIntervalMS);
// if (demoMode) {
//     connect(timerQuery, &QTimer::timeout, this, &OPCReader::queryServerDemo);
// }
// else {
//     connect(timerQuery, &QTimer::timeout, this, &OPCReader::queryServer);
// }

//   return true;
// }

// void OPCReader::queryServer() {
//   // nur zur sicherheit
//   if (demoTestDataMode) {
//     // hier kommen daten über den update slot
//     return;
//   }

//   std::list<std::string> lstNames;
//   std::list<std::string> lstLabels;
//   std::list<std::variant<int, float>> lstData;

//   // SYNCED read on Group
//   COPCItem_DataMap opcData;
//   try {
//     spdlog::trace("opc group read of {} items", itemsCreated.size());
//     group->readSync(itemsCreated, opcData, OPC_DS_DEVICE);
//   } catch (OPCException& ex) {
//     spdlog::warn("reading opc items failed, reason: {}", ex.reasonString());
//     // QLOG_WARN() << "reading opc items failed: reason:" <<
//     // QString::fromStdString(ex.reasonString());
//     return;
//   }

//   POSITION pos = opcData.GetStartPosition();

//   std::map<std::string, std::variant<int, double, std::string>> mapOPCData;
//   std::map<std::string, std::string> mapOPCDataLabels;
//   std::string line_proto = "opc ";
//   while (pos != nullptr) {
//     COPCItem* item = opcData.GetKeyAt(pos);
//     std::string itemName = item->getName();

//     if (!mapOpcItems.contains(item)) {
//       continue;
//     }
//     OPCItem opcItem = mapOpcItems.at(item);
//     OPCItemData* data = opcData.GetNextValue(pos);
//     std::variant<int, double, std::string> var;
//     switch (opcItem.dataType) {
//       case OPCDataType::INT:
//       case OPCDataType::WORD:
//         var = data->vDataValue.iVal;
//         spdlog::trace("name: {} --> value: {}", opcItem.name, std::get<int>(var));
//         // QLOG_TRACE() << QString("%1: %2").arg(opcItem.name).arg(var.toInt(), 6);
//         // line_proto.append(fmt::format("{}={},", opcItem.name.toStdString(), var.toInt()));
//         break;
//       case OPCDataType::STRING: {
//         int wslen = ::SysStringLen(data->vDataValue.bstrVal);
//         int len = ::WideCharToMultiByte(CP_ACP, 0, (wchar_t*)data->vDataValue.bstrVal, wslen, NULL,
//                                         0, NULL, NULL);

//         std::string dblstr(len, '\0');
//         len = ::WideCharToMultiByte(CP_ACP, 0, (wchar_t*)data->vDataValue.bstrVal, wslen,
//                                     &dblstr[0], len, NULL, NULL);
//         var = dblstr;
//       }
//         spdlog::trace("name: {} --> value: {}", opcItem.name, std::get<std::string>(var));
//         // QLOG_TRACE() << QString("%1: %2").arg(opcItem.name).arg(var.toString());
//         // line_proto.append(fmt::format("{}={},", opcItem.name.toStdString(),
//         // var.toString().toStdString()));
//         break;
//       case OPCDataType::FLOAT:
//         var = data->vDataValue.fltVal;
//         spdlog::trace("name: {} --> value: {}", opcItem.name, std::get<double>(var));
//         // QLOG_TRACE() << QString("%1: %2").arg(opcItem.name).arg(var.toDouble(), 10, 'f', 4);
//         // line_proto.append(fmt::format("{}={},", opcItem.name.toStdString(), var.toDouble()));
//         break;
//       default:
//         break;
//         // var = QVariant(0);
//     }

//     mapOPCData.insert({opcItem.name, var});
//     mapOPCDataLabels.insert({opcItem.name, opcItem.label});
//     lstNames.emplace_back(opcItem.name);
//     lstLabels.emplace_back(opcItem.label);
//     lstData.emplace_back(var);

//     spdlog::debug("opc reader: {} = {}", opcItem.name, var);
//     // QLOG_DEBUG() << QString("opc reader") << opcItem.name << var;
//   }
//   // remove last , from trace string
//   line_proto.pop_back();
//   //   auto now = QDateTime::currentDateTime().toMSecsSinceEpoch();
//   //   line_proto.append(fmt::format(" {}000000", now));
//   //   _logger_opc->info(line_proto);

//   //   emit opcDataUpdate(readerName, lstNames, lstLabels, lstData);
//   //   emit opcAsVariantMap(readerName, mapOPCData, mapOPCDataLabels);
// }

// void OPCReader::queryServerDemo() {
//   // nur zur sicherheit
//   if (demoTestDataMode) {
//     // hier kommen daten über den update slot
//     return;
//   }

//   QStringList lstNames;
//   QStringList lstLabels;
//   QVariantList lstData;

//   QVariantMap mapOPCData;
//   QMap<QString, QString> mapOPCDataLabels;
//   QString randomSource("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
//   const int strLen = randomSource.length() / 4;
//   std::string line_proto = "opc ";
//   for (auto item : lstOpcItems) {
//     QVariant var;
//     switch (item.dataType) {
//       case OPCDataType::INT:
//       case OPCDataType::BYTE:
//       case OPCDataType::WORD:
//         var = QRandomGenerator::global()->bounded(255);
//         QLOG_DEBUG() << QString("%1: %2").arg(item.name).arg(var.toInt());
//         line_proto.append(fmt::format("{}={},", item.name.toStdString(), var.toInt()));
//         break;
//       case OPCDataType::FLOAT:
//         var = QRandomGenerator::global()->generateDouble();
//         QLOG_DEBUG() << QString("%1: %2").arg(item.name).arg(var.toDouble());
//         line_proto.append(fmt::format("{}={},", item.name.toStdString(), var.toDouble()));
//         break;
//       case OPCDataType::STRING: {
//         QString str;
//         for (int i = 0; i < strLen; i++) {
//           str.append(
//             randomSource.at(QRandomGenerator::global()->bounded(0, randomSource.length() - 1)));
//         }
//         var = str;
//         QLOG_DEBUG() << QString("%1: %2").arg(item.name).arg(var.toString());
//         line_proto.append(
//           fmt::format("{}={},", item.name.toStdString(), var.toString().toStdString()));
//       } break;
//       case OPCDataType::UNKNOWN:
//         QLOG_DEBUG() << QString("%1: >>unkonwn type !!!<<").arg(item.name);
//         break;
//     }
//     if (!var.isNull()) {
//       mapOPCData.insert(item.name, var);
//     }

//     mapOPCDataLabels.insert(item.name, item.label);
//     lstNames.append(item.name);
//     lstLabels.append(item.label);
//     lstData.append(var);

//     QLOG_DEBUG() << QString("opc reader") << item.name << var;
//     //        QLOG_DEBUG() << QString("opc reader") << item.name << item.dataType << var;
//   }

//   // remove last , from trace string
//   line_proto.pop_back();
//   auto now = QDateTime::currentDateTime().toMSecsSinceEpoch();
//   line_proto.append(fmt::format(" {}000000", now));
//   _logger_opc->info(line_proto);

//   emit opcDataUpdate(readerName, lstNames, lstLabels, lstData);
//   emit opcAsVariantMap(readerName, mapOPCData, mapOPCDataLabels);
// }

// void OPCReader::startQueryServer(bool canStart) {
//   if (canStart) {
//     QLOG_INFO() << "opc reader: starting to query the server";
//     timerReStart->stop();
//     timerQuery->start();
//   } else {
//     if (!timerReStart->isActive()) {
//       timerReStart->start();
//     }
//   }
// }

// void OPCReader::startOPC() {
//   spdlog::info("opc reader trying to establish connection to server {}", opcServerName);
//   //   QLOG_INFO() << "opc reader: trying to establish connection to opc server" << opcServerName;
//   COPCClient::init();

//   // wir verbinden uns immer zu einem server der auf demselben rechner läuft
//   std::string hostName = QHostInfo::localHostName();
//   host = COPCClient::makeHost(hostName.toStdString());

//   // list available opc servers
//   std::vector<std::string> localServerList;
//   try {
//     host->getListOfDAServers(IID_CATID_OPCDAServer20, localServerList);
//   } catch (OPCException& ex) {
//     QString strReason = QString::fromStdString(ex.reasonString());
//     QLOG_WARN() << "could not connect to OPC server" << QString::fromStdString(ex.reasonString());
//     emit startOPCInfo(false);
//     return;
//   }
//   if (localServerList.empty()) {
//     QString msg = tr("no opc DA servers available on %1").arg(hostName);
//     if (errorOnHostNotFound) {
//       QLOG_FATAL() << QString("opc reader") << msg;
//     } else {
//       QLOG_WARN() << QString("opc reader") << msg;
//     }
//     emit startOPCInfo(false);
//     return;
//   }

//   // gibt es unseren server
//   QStringList lstLocalServers;
//   for (uint i = 0; i < localServerList.size(); i++) {
//     lstLocalServers << QString::fromStdString(localServerList.at(i));
//   }
//   if (!lstLocalServers.contains(opcServerName)) {
//     QString msg = tr("opc server with name %1 not found").arg(opcServerName);
//     if (errorOnHostNotFound) {
//       QLOG_FATAL() << QString("opc reader") << msg;
//     } else {
//       QLOG_WARN() << QString("opc reader") << msg;
//     }
//     emit startOPCInfo(false);
//     return;
//   }

//   // connect to opc server
//   try {
//     opcServer = host->connectDAServer(opcServerName.toStdString());
//   } catch (OPCException& ex) {
//     QString strReason = QString::fromStdString(ex.reasonString());
//     QLOG_WARN() << "could not connect to OPC server" << QString::fromStdString(ex.reasonString());
//     emit startOPCInfo(false);
//     return;
//   }

//   // Check status
//   ServerStatus status;
//   opcServer->getStatus(status);
//   QString strStatus = QString("%1 server state is %2").arg(opcServerName).arg(status.dwServerState);
//   if (status.dwServerState == 1) {
//     strStatus.append(" running");
//   }
//   QLOG_INFO() << QString("opc reader") << strStatus;

//   // make group
//   unsigned long refreshRate;
//   group = opcServer->makeGroup("Group", true, static_cast<unsigned long>(queryIntervalMS),
//                                refreshRate, 0.0);
//   if (refreshRate != static_cast<unsigned long>(queryIntervalMS)) {
//     QString msg = tr("%1 requested update rate was %2 but got %3")
//                     .arg(opcServerName)
//                     .arg(queryIntervalMS)
//                     .arg(refreshRate);
//     QLOG_WARN() << QString("opc reader") << msg;
//     if (static_cast<int>(refreshRate) > queryIntervalMS) {
//       queryIntervalMS = static_cast<int>(refreshRate);
//       timerQuery->setInterval(queryIntervalMS);
//       QLOG_INFO() << "opc reader: adjusting query interval time to" << queryIntervalMS
//                   << "milliseconds";
//     }
//   }

//   // add our items to group
//   for (int i = 0; i < lstOpcItems.size(); i++) {
//     OPCItem item = lstOpcItems.at(i);
//     std::string myItemName = item.name.toStdString();
//     try {
//       COPCItem* myItem = group->addItem(myItemName, true);
//       itemsCreated.push_back(myItem);
//       mapOpcItems[myItem] = item;
//     } catch (OPCException& ex) {
//       QLOG_WARN() << "could not add OPC item" << QString::fromStdString(myItemName)
//                   << "reason:" << QString::fromStdString(ex.reasonString());
//     }
//   }
//   if (itemsCreated.size() != static_cast<size_t>(lstOpcItems.size())) {
//     QLOG_ERROR() << QString("opc reader") << itemsCreated.size() << "out of" << lstOpcItems.size()
//                  << "opc items created";
//   } else {
//     QLOG_INFO() << QString("opc reader") << itemsCreated.size() << "out of" << lstOpcItems.size()
//                 << "opc items created";
//   }

//   if (itemsCreated.empty()) {
//     QLOG_ERROR() << "opc reader: non of the querry items is available on server!";
//     QLOG_ERROR() << "opc reader: shutting down";
//     emit startOPCInfo(false);
//     return;
//   }

//   emit startOPCInfo(true);
// }
