#include <iostream>

#include "FileWatcher.h"

int main() {
	watcher::FileWatchar fileWatchar;
	fileWatchar.addPath("C:/Users/CYS/Downloads/temp2.txt"); // Add path that want to chage(Created, Modified, Erased)
	fileWatchar.addPath("C:/Users/CYS/Downloads/temp1.txt");

	fileWatchar.start([](std::filesystem::path path, watcher::FileStatus status) { // callback
		switch (status)
		{
		case watcher::FileStatus::Created:
			std::cout << path.string() << " Created!" << std::endl;
			break;
		case watcher::FileStatus::Modified:
			std::cout << path.string() << " Modified!" << std::endl;
			break;
		case watcher::FileStatus::Erased:
			std::cout << path.string() << " Erased!" << std::endl;
			break;
		default:
			std::cout << "error!" << std::endl;
			break;
		}
		});

}