#pragma once
#include <mysql++/mysql++.h>
#include <unordered_set>
#include "geosick/config.hpp"
#include "geosick/geo_row_reader.hpp"

namespace geosick {

class MysqlReader;

class MysqlDb {
  mysqlpp::Connection m_conn;
public:
  explicit MysqlDb(const Config& cfg);
  std::unique_ptr<MysqlReader> read_rows();

  struct UserIds {
      std::unordered_set<uint32_t> sick;
      std::unordered_set<uint32_t> query;
  };
  UserIds read_user_ids();
};

class MysqlReader final: public GeoRowReader {
    mysqlpp::UseQueryResult m_result;
public:
    explicit MysqlReader(mysqlpp::UseQueryResult result): m_result(result) {}
    virtual std::optional<GeoRow> read() override;
};

}
