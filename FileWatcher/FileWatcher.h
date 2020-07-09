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
		constexpr int kDefaultFileWatchingTime = 500;
	}
	enum class FileStatus {
		Created,
		Modified,
		Erased,
	};

	class FileWatchar {
	public:
		FileWatchar() :running_(false) {}
		~FileWatchar() {
			stop();
		}

		void start(std::function<void(std::filesystem::path, FileStatus)> globalCallback = nullptr) {
			if (running_.exchange(true) == true)
				return;

			if (globalCallback != nullptr) {
				std::lock_guard lock(watchingMutex_);
				for (auto& it : file_watching_map_) {
					it.second.callbacks.push_back(globalCallback);
				}
				globalCallback_ = globalCallback;
			}

			watchingThread_ = std::thread([&]() {
				while (running_) {
					std::this_thread::sleep_for(std::chrono::milliseconds(kDefaultFileWatchingTime));

					std::lock_guard lock(watchingMutex_);

					for (auto& it : file_watching_map_) {
						auto path = std::filesystem::path(it.first); // key
						auto& data = it.second; // value			

						if (std::filesystem::exists(path)) {
							if (data.time == std::filesystem::file_time_type::min()) {
								DoCallback(data.callbacks, path, FileStatus::Created);
							}
							else if (data.time != std::filesystem::last_write_time(path)) {
								DoCallback(data.callbacks, path, FileStatus::Modified);
							}
							data.time = std::filesystem::last_write_time(path);
						}
						else if (data.time != std::filesystem::file_time_type::min()) {
							data.time = std::filesystem::file_time_type::min();
							DoCallback(data.callbacks, path, FileStatus::Erased);
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

		void addPath(std::filesystem::path path, std::function<void(std::filesystem::path, FileStatus)> function = nullptr) {
			std::lock_guard lock(watchingMutex_);
			Data data;
			if (std::filesystem::exists(path)) {
				data.time = std::filesystem::last_write_time(path);
			}
			else {
				data.time = std::filesystem::file_time_type::min();
			}
			file_watching_map_.emplace(path.string(), data);

			auto& callbacks = file_watching_map_[path.string()].callbacks;

			if (function != nullptr)
				callbacks.push_back(function);
			if (globalCallback_ != nullptr) {
				callbacks.push_back(globalCallback_);
			}
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
			std::vector<std::function<void(std::filesystem::path, FileStatus)>> callbacks;
		private:
		};

		void DoCallback(const std::vector<std::function<void(std::filesystem::path, FileStatus)>> callbacks, const std::filesystem::path& path, const FileStatus& status) const {
			for (auto& callback : callbacks) {
				callback(path, status);
			}
		}

		std::thread watchingThread_;
		std::mutex watchingMutex_;
		std::unordered_map<std::string, Data> file_watching_map_;
		std::function<void(std::filesystem::path, FileStatus)> globalCallback_ = nullptr;

		std::atomic<bool> running_;
	};
}
#endif // !FILE_WATCHER_H