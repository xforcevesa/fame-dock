#include "defs.h"
#include "errors.h"
// #include "parser/yacc.tab.h"
//  #include "parser/yacc.tab.h"
//   #include "parser/yacc.tab.h"
#include <optional>
#include <ostream>
#include <unistd.h>
#include <vector>
#define private public
#include "common/config.h"
#include "index/ix_manager.h"
#include "optimizer/plan.h"
#include "optimizer/planner.h"
#include "parser/ast.h"
#include "record/rm_defs.h"
// #include "parser/yacc.tab.h"
#include "record/rm_scan.h"
#include "system/sm_meta.h"

#include "record/rm_file_handle.h"
#include "record/rm_manager.h"
#include "storage/buffer_pool_manager.h"
#include "storage/disk_manager.h"
#include "storage/page.h"
#undef private
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <system/sm_manager.h>
#include <unordered_map>
const std::string TEST_TABLE_NAME = "t1";
const std::string TEST_DB_NAME = "grade";
const std::string TEST_FILE_NAME = "basic";
constexpr int MAX_FILES = 32;
constexpr int MAX_PAGES = 128;
constexpr size_t TEST_BUFFER_POOL_SIZE = MAX_FILES * MAX_PAGES;
auto disk_manager = std::make_unique<DiskManager>();
auto buffer_pool_manager = std::make_unique<BufferPoolManager>(
    TEST_BUFFER_POOL_SIZE, disk_manager.get());
auto rm_manager =
    std::make_unique<RmManager>(disk_manager.get(), buffer_pool_manager.get());
auto ix_manager =
    std::make_unique<IxManager>(disk_manager.get(), buffer_pool_manager.get());
auto sm_manager =
    std::make_unique<SmManager>(disk_manager.get(), buffer_pool_manager.get(),
                                rm_manager.get(), ix_manager.get());
std::unordered_map<int, char *> mock;
char *mock_get_page(int fd, int page_no) {
  return &mock[fd][page_no * PAGE_SIZE];
}
void check_disk(int fd, int page_no) {
  char buf[PAGE_SIZE];
  disk_manager->read_page(fd, page_no, buf, PAGE_SIZE);
  char *mock_buf = mock_get_page(fd, page_no);
  assert(memcmp(mock_buf, buf, PAGE_SIZE) == 0);
}
void check_disk_all() {
  for (auto &file : mock) {
    auto &[fd, _] = file;
    for (int page_no = 0; page_no < MAX_PAGES; page_no++) {
      check_disk(fd, page_no);
    }
  }
}
void check_cache(int fd, int page_no) {
  Page *page = buffer_pool_manager->fetch_page(PageId{fd, page_no});
  char *mock_buf = mock_get_page(fd, page_no);
  assert(memcmp(mock_buf, page->get_data(), PAGE_SIZE) == 0);
  buffer_pool_manager->unpin_page(PageId{fd, page_no}, false);
}
void check_cache_all() {
  for (auto &file : mock) {
    auto &[fd, _] = file;
    for (int page_no = 0; page_no < MAX_PAGES; page_no++) {
      check_cache(fd, page_no);
    }
  }
}
void rand_buf(int size, char *buf) {
  for (int i = 0; i < size; i++) {
    int rand_ch = rand() & 0xff;
    buf[i] = rand_ch;
  }
}
int rand_fd() {
  assert(mock.size() == MAX_FILES);
  int fd_idx = rand() % MAX_FILES;
  auto it = mock.begin();
  for (int i = 0; i < fd_idx; i++) {
    it++;
  }
  return it->first;
}
struct rid_hash_t {
  size_t operator()(const Rid &rid) const {
    return (rid.page_no << 16) | rid.slot_no;
  }
};
struct rid_equal_t {
  bool operator()(const Rid &x, const Rid &y) const {
    return x.page_no == y.page_no && x.slot_no == y.slot_no;
  }
};
void check_equal(
    const RmFileHandle *file_handle,
    const std::unordered_map<Rid, std::string, rid_hash_t, rid_equal_t> &mock) {
  for (auto &entry : mock) {
    auto &[rid, mock_buf_str] = entry;
    auto rec = file_handle->get_record(rid, nullptr);
    assert(memcmp(mock_buf_str.c_str(), rec->data,
                  file_handle->file_hdr_.record_size) == 0);
  }
  for (int i = 0; i < 10; i++) {
    Rid rid{1 + rand() % (file_handle->file_hdr_.num_pages - 1),
            rand() % file_handle->file_hdr_.num_records_per_page};
    bool mock_exist = mock.count(rid) > 0;
    bool rm_exist = file_handle->is_record(rid);
    assert(mock_exist == rm_exist);
  }
  int num_records = 0;
  for (RmScan scan(file_handle); !scan.is_end(); scan.next()) {
    assert(mock.count(scan.rid()) > 0);
    auto rec = file_handle->get_record(scan.rid(), nullptr);
    assert(memcmp(rec->data, mock.at(scan.rid()).c_str(),
                  file_handle->file_hdr_.record_size) == 0);
    num_records++;
  }
  assert(num_records == mock.size());
}
std::ostream &operator<<(std::ostream &os, const Rid &rid) {
  return os << '(' << rid.page_no << ',' << rid.slot_no << ')';
}
class CreateTableTest : public ::testing::Test {
public:
  std::unique_ptr<SmManager> sm_manager_;
  std::unique_ptr<DiskManager> disk_manager_;
  std::unique_ptr<BufferPoolManager> buffer_pool_manager_;
  std::unique_ptr<RmManager> rm_manager_;
  std::unique_ptr<IxManager> ix_manager_;
  int fd = -1;

public:
  void SetUp() override {
    ::testing::Test::SetUp();
    disk_manager_ = std::make_unique<DiskManager>();
    if (!disk_manager_->is_dir(TEST_DB_NAME)) {
      disk_manager_->create_dir(TEST_DB_NAME);
    }
    assert(disk_manager_->is_dir(TEST_DB_NAME));
    if (chdir(TEST_DB_NAME.c_str()) < 0) {
      printf("Throw error at file: %s, line: %d\n", __FILE__, __LINE__);

      throw UnixError();
    }
    if (disk_manager_->is_file(TEST_FILE_NAME)) {
      disk_manager_->destroy_file(TEST_FILE_NAME);
    }
    disk_manager_->create_file(TEST_FILE_NAME);
    assert(disk_manager_->is_file(TEST_FILE_NAME));
    fd = disk_manager_->open_file(TEST_FILE_NAME);

    assert(fd != -1);
  }
  void TearDown() override {
    // sm_manager_->drop_table(TEST_TABLE_NAME, nullptr);

    disk_manager_->close_file(fd);
    if (chdir("..") < 0) {
      printf("Throw error at file: %s, line: %d\n", __FILE__, __LINE__);

      throw UnixError();
    }
    assert(disk_manager_->is_dir(TEST_DB_NAME));
  };
};
TEST_F(CreateTableTest, SimpleTest) {
  // constexpr int buffer_pool_size = 10;
  auto disk_manager_ = CreateTableTest::disk_manager_.get();
  auto buffer_pool_manager_ =
      std::make_unique<BufferPoolManager>(10, disk_manager_);
  auto rm_manager_ =
      std::make_unique<RmManager>(disk_manager_, buffer_pool_manager_.get());
  auto ix_manager_ =
      std::make_unique<IxManager>(disk_manager_, buffer_pool_manager_.get());
  sm_manager_ =
      std::make_unique<SmManager>(disk_manager_, buffer_pool_manager_.get(),
                                  rm_manager_.get(), ix_manager_.get());
  if (disk_manager_->is_dir(TEST_DB_NAME)) {
    disk_manager_->destroy_dir(TEST_DB_NAME);
  }
  // 清除文件夹之后，直接用sm_manager_调用create_db就可以
  sm_manager_->create_db(TEST_DB_NAME);
  EXPECT_EQ(sm_manager_->is_dir(TEST_DB_NAME), true);
  std::vector<ColDef> col_defs;
  col_defs.push_back(ColDef{"id1", TYPE_INT, sizeof(int)});
  // string的实现是一个字符数组
  col_defs.push_back(ColDef{"name", TYPE_STRING, 5 * sizeof(char)});
  sm_manager_->create_table(TEST_TABLE_NAME, col_defs, nullptr);
  ASSERT_NE(sm_manager_->db_.tabs_.count(TEST_TABLE_NAME), 0);
  EXPECT_EQ(sm_manager_->db_.tabs_[TEST_TABLE_NAME].cols.size(), 2);
  // sm_manager_->show_tables(nullptr);
}
TEST_F(CreateTableTest, DROP_TEST) {
  auto disk_manager_ = CreateTableTest::disk_manager_.get();
  auto buffer_pool_manager_ =
      std::make_unique<BufferPoolManager>(10, disk_manager_);
  auto rm_manager_ =
      std::make_unique<RmManager>(disk_manager_, buffer_pool_manager_.get());
  auto ix_manager_ =
      std::make_unique<IxManager>(disk_manager_, buffer_pool_manager_.get());
  sm_manager_ =
      std::make_unique<SmManager>(disk_manager_, buffer_pool_manager_.get(),
                                  rm_manager_.get(), ix_manager_.get());
  if (disk_manager_->is_dir(TEST_DB_NAME)) {
    disk_manager_->destroy_dir(TEST_DB_NAME);
  }
  // 手动删掉
  sm_manager_->db_.tabs_.erase(TEST_TABLE_NAME);
  std::vector<ColDef> col_defs;
  col_defs.push_back(ColDef{"id", TYPE_FLOAT, sizeof(float)});
  col_defs.push_back(ColDef{"name", TYPE_STRING, 7 * sizeof(char)});
  col_defs.push_back(ColDef{"age", TYPE_INT, sizeof(int)});
  sm_manager_->create_table(TEST_TABLE_NAME, col_defs, nullptr);
  ASSERT_NE(sm_manager_->db_.tabs_[TEST_TABLE_NAME].cols.size(), 0);
  EXPECT_EQ(sm_manager_->db_.tabs_[TEST_TABLE_NAME].cols[0].name, "id");
  sm_manager_->drop_table(TEST_TABLE_NAME, nullptr);
  EXPECT_EQ(sm_manager_->db_.tabs_.count(TEST_TABLE_NAME), 0);
}
TEST_F(CreateTableTest, INDEX_TEST) {
  auto disk_manager_ = CreateTableTest::disk_manager_.get();
  auto buffer_pool_manager_ =
      std::make_unique<BufferPoolManager>(10, disk_manager_);
  auto rm_manager_ =
      std::make_unique<RmManager>(disk_manager_, buffer_pool_manager_.get());
  auto ix_manager_ =
      std::make_unique<IxManager>(disk_manager_, buffer_pool_manager_.get());
  sm_manager_ =
      std::make_unique<SmManager>(disk_manager_, buffer_pool_manager_.get(),
                                  rm_manager_.get(), ix_manager_.get());
  if (disk_manager_->is_dir(TEST_DB_NAME)) {
    disk_manager_->destroy_dir(TEST_DB_NAME);
  }
  sm_manager_->db_.tabs_.erase(TEST_TABLE_NAME);
  // 测试create index, drop index
  std::vector<ColDef> col_defs;
  col_defs.push_back(ColDef{"id1", TYPE_FLOAT, sizeof(float)});
  col_defs.push_back(ColDef{"name", TYPE_STRING, sizeof(char) * 7});
  col_defs.push_back(ColDef{"avaerage", TYPE_INT, sizeof(int)});
  sm_manager_->create_table(TEST_TABLE_NAME, col_defs, nullptr);
  // 验证插入是否成功
  EXPECT_EQ(sm_manager_->db_.tabs_[TEST_TABLE_NAME].cols.size(), 3);
  std::vector<std::string> col_names;
  col_names.push_back("id1");
  col_names.push_back("name");
  sm_manager_->create_index(TEST_TABLE_NAME, col_names, nullptr);
  ASSERT_EQ(sm_manager_->ix_manager_->exists(TEST_TABLE_NAME, col_names), true);
  sm_manager_->drop_index(TEST_TABLE_NAME, col_names, nullptr);
  ASSERT_NE(sm_manager_->ix_manager_->exists(TEST_TABLE_NAME, col_names), true);
}
TEST_F(CreateTableTest, ADD_COL_TEST) {
  auto disk_manager_ = CreateTableTest::disk_manager_.get();
  auto buffer_pool_manager_ =
      std::make_unique<BufferPoolManager>(10, disk_manager_);
  auto rm_manager_ =
      std::make_unique<RmManager>(disk_manager_, buffer_pool_manager_.get());
  auto ix_manager_ =
      std::make_unique<IxManager>(disk_manager_, buffer_pool_manager_.get());
  sm_manager_ =
      std::make_unique<SmManager>(disk_manager_, buffer_pool_manager_.get(),
                                  rm_manager_.get(), ix_manager_.get());
  if (disk_manager_->is_dir(TEST_DB_NAME)) {
    disk_manager_->destroy_dir(TEST_DB_NAME);
  }
  sm_manager_->db_.tabs_.erase(TEST_TABLE_NAME);
  // 测试create index, drop index
  std::vector<ColDef> col_defs;
  col_defs.push_back(ColDef{"id1", TYPE_FLOAT, sizeof(float)});
  col_defs.push_back(ColDef{"name", TYPE_STRING, sizeof(char) * 7});
  col_defs.push_back(ColDef{"avaerage", TYPE_INT, sizeof(int)});
  sm_manager_->create_table(TEST_TABLE_NAME, col_defs, nullptr);
  EXPECT_EQ(sm_manager_->db_.tabs_[TEST_TABLE_NAME].cols.size(), 3);
  col_defs.clear();
  col_defs.push_back(ColDef{"new_id", TYPE_INT, sizeof(int)});
  col_defs.push_back(ColDef{"name2", TYPE_STRING, 7 * sizeof(char)});
  ASSERT_NE(sm_manager_->add_table_cols(TEST_TABLE_NAME, col_defs, nullptr),
            std::make_optional<int>(0));
  EXPECT_EQ(sm_manager_->db_.tabs_[TEST_TABLE_NAME].cols.size(), 5);
}
