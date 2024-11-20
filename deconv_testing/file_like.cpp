#include "file_like.hpp"

FileLike::FileLike(){
	file_name = "";
	bytes = std::vector<std::byte>(0);
	pos = 0;
	is_open=false;
	contents_changed=false;
}

FileLike::FileLike(const std::string& _file_name, const std::vector<std::byte>& _bytes){
	file_name = _file_name;
	bytes = _bytes;
	pos = 0;
	is_open=true;
	contents_changed=false;
}


void FileLike::set_open_with_contents(const std::vector<std::byte>& new_bytes, bool _contents_changed){
	bytes = new_bytes; // this should be a copy operation
	contents_changed = _contents_changed; // we have reset the contents externally, so it has not "changed"
	is_open=true;
	pos=0;
}