# FileWatcher
FileWatcher is simple file change detection that based c++17

This F

### sample code 

```cpp
#include <iostream>

#include "FileWatcher.h"

int main() {
	watcher::FileWatchar fileWatchar;
	// Add path that want to chage(Created, Modified, Erased)
	// Support watching that not created file.
	fileWatchar.addPath("C:/Users/CYS/Downloads/temp2.txt"); 
	fileWatchar.addPath("C:/Users/CYS/Downloads/temp1.txt");

	fileWatchar.start([](std::filesystem::path path, watcher::FileStatus status) { // Callback
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
	std::this_thread::sleep_for(std::chrono::milliseconds(5000));
	fileWatchar.addPath("C:/Users/CYS/Downloads/temp.txt"); // Support after start(). also thread-safe. start watching
	std::this_thread::sleep_for(std::chrono::milliseconds(5000));
	fileWatchar.removePath("C:/Users/CYS/Downloads/temp.txt"); // Support after start(). also thread-safe. stop watching

	std::this_thread::sleep_for(std::chrono::milliseconds(10000000)); // Do something
}
```


### sample image

![sample](./img/sample.png)
