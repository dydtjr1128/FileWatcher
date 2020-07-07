#ifndef FILE_WATCHER_H
#define FILE_WATCHER_H

#include <filesystem>
#include <functional>
#include <unordered_map>
#include <thread>

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
		void start(std::function<void(std::filesystem::path, FileStatus)> function) {
			while (running_) {
				std::this_thread::sleep_for(std::chrono::milliseconds(kDefaultFileWatchingTime));

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
		void addPath(std::filesystem::path path) {
			if (std::filesystem::exists(path)) {
				file_watching_map_[path.string()] = std::filesystem::last_write_time(path);
			}
			else {
				file_watching_map_[path.string()] = std::filesystem::file_time_type::min();
			}
		}
	private:
		std::unordered_map<std::string, std::filesystem::file_time_type> file_watching_map_;

		bool running_;

	};
}
#endif // !FILE_WATCHER_H