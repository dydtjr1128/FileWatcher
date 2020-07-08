#ifndef FILE_WATCHER_H
#define FILE_WATCHER_H

#include <filesystem>
#include <functional>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace watcher {
	namespace {
		constexpr int kDefaultFileWatchingTime = 500;
	}
	enum class FileStatus {
		Created,
		Modified,
		Erased,
	};

	class FileWatchar {
	public:
		FileWatchar() :running_(true) {}
		~FileWatchar() {
			running_ = false;
			watchingThread_.join();
		}
		void start(std::function<void(std::filesystem::path, FileStatus)> function) {
			watchingThread_ = std::thread([&, function]() {
				while (running_ && function) {
					std::this_thread::sleep_for(std::chrono::milliseconds(kDefaultFileWatchingTime));

					std::lock_guard lock(watchingMutex_);

					for (const auto& it : file_watching_map_) {
						bool exist = std::filesystem::exists(it.first);
						if (exist) {
							if (it.second == std::filesystem::file_time_type::min()) {
								function(std::filesystem::path(it.first), FileStatus::Created);
							}
							else if (it.second != std::filesystem::last_write_time(it.first)) {
								function(std::filesystem::path(it.first), FileStatus::Modified);
							}
							file_watching_map_[it.first] = std::filesystem::last_write_time(it.first);
						}
						else if (it.second != std::filesystem::file_time_type::min()) {
							file_watching_map_[it.first] = std::filesystem::file_time_type::min();
							function(std::filesystem::path(it.first), FileStatus::Erased);
						}
					}
				}
				}
			);
		}
		void addPath(std::filesystem::path path) {
			std::lock_guard lock(watchingMutex_);
			if (std::filesystem::exists(path)) {
				file_watching_map_[path.string()] = std::filesystem::last_write_time(path);
			}
			else {
				file_watching_map_[path.string()] = std::filesystem::file_time_type::min();
			}
		}
		void removePath(std::filesystem::path path) {
			std::lock_guard lock(watchingMutex_);
			if (file_watching_map_.find(path.string()) != file_watching_map_.end()) {
				file_watching_map_.erase(path.string());
			}
		}
	private:
		std::thread watchingThread_;
		std::mutex watchingMutex_;
		std::unordered_map<std::string, std::filesystem::file_time_type> file_watching_map_;

		bool running_;
	};
}
#endif // !FILE_WATCHER_H