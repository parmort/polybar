#pragma once

#include <moodycamel/blockingconcurrentqueue.h>

#include <thread>

#include "common.hpp"
#include "components/types.hpp"
#include "events/signal_fwd.hpp"
#include "events/signal_receiver.hpp"
#include "events/types.hpp"
#include "settings.hpp"
#include "utils/actions.hpp"
#include "utils/file.hpp"
#include "x11/types.hpp"

POLYBAR_NS

// fwd decl {{{

enum class alignment;
class bar;
class config;
class connection;
class inotify_watch;
class ipc;
class logger;
class signal_emitter;
namespace modules {
  struct module_interface;
}  // namespace modules
using module_t = shared_ptr<modules::module_interface>;
using modulemap_t = std::map<alignment, vector<module_t>>;

// }}}

class controller
    : public signal_receiver<SIGN_PRIORITY_CONTROLLER, signals::eventqueue::exit_terminate,
          signals::eventqueue::exit_reload, signals::eventqueue::notify_change, signals::eventqueue::notify_forcechange,
          signals::eventqueue::check_state, signals::ipc::action, signals::ipc::command, signals::ipc::hook,
          signals::ui::ready, signals::ui::button_press, signals::ui::update_background> {
 public:
  using make_type = unique_ptr<controller>;
  static make_type make(unique_ptr<ipc>&& ipc, unique_ptr<inotify_watch>&& config_watch);

  explicit controller(connection&, signal_emitter&, const logger&, const config&, unique_ptr<bar>&&, unique_ptr<ipc>&&,
      unique_ptr<inotify_watch>&&);
  ~controller();

  bool run(bool writeback, string snapshot_dst);

  bool enqueue(event&& evt);
  bool enqueue(string&& input_data);

 protected:
  void read_events();
  void process_eventqueue();
  void process_inputdata();
  bool process_update(bool force);

  bool on(const signals::eventqueue::notify_change& evt);
  bool on(const signals::eventqueue::notify_forcechange& evt);
  bool on(const signals::eventqueue::exit_terminate& evt);
  bool on(const signals::eventqueue::exit_reload& evt);
  bool on(const signals::eventqueue::check_state& evt);
  bool on(const signals::ui::ready& evt);
  bool on(const signals::ui::button_press& evt);
  bool on(const signals::ipc::action& evt);
  bool on(const signals::ipc::command& evt);
  bool on(const signals::ipc::hook& evt);
  bool on(const signals::ui::update_background& evt);

 private:
  size_t setup_modules(alignment align);

  bool forward_action(const actions_util::action& cmd);
  bool try_forward_legacy_action(const string& cmd);

  void switch_module_visibility(string module_name_raw, int visible);

  connection& m_connection;
  signal_emitter& m_sig;
  const logger& m_log;
  const config& m_conf;
  unique_ptr<bar> m_bar;
  unique_ptr<ipc> m_ipc;
  unique_ptr<inotify_watch> m_confwatch;

  array<unique_ptr<file_descriptor>, 2> m_queuefd{};

  /**
   * \brief State flag
   */
  std::atomic<bool> m_process_events{false};

  /**
   * \brief Destination path of generated snapshot
   */
  string m_snapshot_dst;

  /**
   * \brief Controls weather the output gets printed to stdout
   */
  bool m_writeback{false};

  /**
   * \brief Internal event queue
   */
  moodycamel::BlockingConcurrentQueue<event> m_queue;

  /**
   * \brief Loaded modules
   */
  vector<module_t> m_modules;

  /**
   * \brief Loaded modules grouped by block
   */
  modulemap_t m_blocks;

  /**
   * \brief Maximum number of subsequent events to swallow
   */
  size_t m_swallow_limit{5U};

  /**
   * \brief Time to wait for subsequent events
   */
  std::chrono::milliseconds m_swallow_update{10};

  /**
   * \brief Input data
   */
  string m_inputdata;

  /**
   * \brief Thread for the eventqueue loop
   */
  std::thread m_event_thread;

  /**
   * \brief Misc threads
   */
  vector<std::thread> m_threads;
};

POLYBAR_NS_END
