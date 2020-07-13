#ifndef FILE_WATCHER_H
#define FILE_WATCHER_H

#include <atomic>
#include <filesystem>
#include <functional>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>

namespace watcher {

	namespace {
		constexpr int kDefaultFileCheckingTime = 500;
	}

	enum class FileStatus {
		Created,
		Modified,
		Erased,
	};

	using Callback = std::function<void(std::filesystem::path, FileStatus)>;

	class FileWatchar {
	public:
		FileWatchar() :running_(false) {}
		virtual ~FileWatchar() {
			stop();
		}

		void start(Callback globalCallback = nullptr) {
			if (running_.exchange(true) == true) {
				return;
			}

			if (globalCallback != nullptr) {
				std::lock_guard lock(watchingMutex_);
				for (auto& it : file_watching_map_) {
					it.second.callbacks.push_back(globalCallback);
				}
				globalCallback_ = globalCallback;
			}

			watchingThread_ = std::thread([&]() {
				while (running_) {
					std::this_thread::sleep_for(std::chrono::milliseconds(kDefaultFileCheckingTime));

					std::lock_guard lock(watchingMutex_);

					for (auto& it : file_watching_map_) {
						auto path = std::filesystem::path(it.first); // key
						auto& data = it.second; // value			

						if (std::filesystem::exists(path)) {
							if (data.time == std::filesystem::file_time_type::min()) {
								DoCallback(data.callbacks, path, FileStatus::Created);
							} else if (data.time != std::filesystem::last_write_time(path)) {
								DoCallback(data.callbacks, path, FileStatus::Modified);
							}
							data.time = std::filesystem::last_write_time(path);
						} else if (data.time != std::filesystem::file_time_type::min()) {
							DoCallback(data.callbacks, path, FileStatus::Erased);
							data.time = std::filesystem::file_time_type::min();
						}
					}
				}
				}
			);
		}

		void stop() {
			running_ = false;
			watchingThread_.join();
			file_watching_map_.clear();
		}

		void addPath(std::filesystem::path path, Callback callback = nullptr) {
			std::lock_guard lock(watchingMutex_);
			Data data;
			if (std::filesystem::exists(path)) {
				data.time = std::filesystem::last_write_time(path);
			} else {
				data.time = std::filesystem::file_time_type::min();
			}

			if (callback != nullptr) {
				data.callbacks.push_back(callback);
			}

			if (globalCallback_ != nullptr) {
				data.callbacks.push_back(globalCallback_);
			}

			file_watching_map_.emplace(path.string(), data);
		}

		void removePath(std::filesystem::path path) {
			std::lock_guard lock(watchingMutex_);
			if (file_watching_map_.find(path.string()) != file_watching_map_.end()) {
				file_watching_map_.at(path.string()).callbacks.clear();
				file_watching_map_.erase(path.string());
			}
		}

	private:
		class Data {
		public:
			std::filesystem::file_time_type time;
			std::vector<Callback> callbacks;
		private:
		};

		void DoCallback(const std::vector<Callback> callbacks, const std::filesystem::path& path, const FileStatus& status) const {
			for (auto& callback : callbacks) {
				callback(path, status);
			}
		}

		std::thread watchingThread_;
		std::mutex watchingMutex_;
		std::unordered_map<std::string, Data> file_watching_map_;
		Callback globalCallback_ = nullptr;

		std::atomic<bool> running_;
	};
}
#endif // !FILE_WATCHER_H