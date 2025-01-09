#include <iostream>
#include <vector>

#include "logging.h"
#include "data_utils.hpp"

namespace du = data_utils;

template<class T, class T2>
void print_array(const std::vector<T>& a, const std::vector<T2> shape){
	for(int i=0; i<shape[0]; ++i){
		for(int j=0; j<shape[1]; ++j){
			if (a[i*shape[0]+j] > 0){
				std::cout << a[i*shape[0]+j];
			} else {
				std::cout << " ";
			}
			std::cout << " ";
		}
		std::cout << std::endl;
	}
}

template<class T1, class T2, class T3>
void set_region_to(std::vector<T1>& a, T3 value, const std::vector<T2> shape, const du::ConnectedRegionNode* node){
	GET_LOGGER;
	LOGV_DEBUG(node);
	LOGV_DEBUG(node->children.size());
	for (auto child : node->children){
		set_region_to(a, value, shape, child);
	}
	LOGV_DEBUG(node->span.y, node->span.x_begin, node->span.x_end);
	for(int i=node->span.x_begin; i<node->span.x_end; ++i){
		a[node->span.y*shape[0]+i] = value;
	}
}

int main(int argc, char** argv){
	INIT_LOGGING("DEBUG");
	GET_LOGGER;
	
	std::cout << "hello world" << std::endl;
	LOG_DEBUG("HELLO WORLD");
	
	
	std::vector<int> test_data_shape{11,11};
	std::vector<bool> test_data_11x11{
	// 0 1 2 3 4 5 6 7 8 9 10 11
		1,0,1,0,1,0,1,0,0,0,0,
		1,0,1,0,1,0,1,0,0,0,0,
		1,1,1,0,1,0,1,0,0,0,0,
		0,0,1,1,1,1,1,0,0,1,1,
		0,0,0,0,0,0,0,1,1,1,1,
		1,1,1,1,0,0,0,0,0,1,1,
		1,0,1,0,1,0,0,0,0,0,0,
		1,1,1,1,1,1,1,1,1,1,1,
		1,1,0,0,0,0,1,1,0,0,1,
		0,1,0,0,0,1,1,1,0,1,1,
		0,0,1,1,0,0,0,0,0,0,0,
	};
	std::vector<uint8_t> test_data_values(test_data_11x11.size());
	
	//std::vector<du::ConnectedRegionNode*> region_trees = du::get_regions(test_data_11x11, test_data_shape);
	du::Regions regions = du::get_regions(test_data_11x11, test_data_shape);
	
	print_array(test_data_values, test_data_shape);
	regions.label(test_data_values, test_data_shape);
	print_array(test_data_values, test_data_shape);
	
	
	/*
	LOGV_DEBUG(region_trees.size());
	
	for (auto item : region_trees){
		LOGV_DEBUG(item->span.y, item->span.x_begin, item->span.x_end);
	}
	
	print_array(test_data_values, test_data_shape);
	for(int i=0; i<region_trees.size(); ++i){
		//set_region_to(test_data_values, i+1, test_data_shape, region_trees[i]);
		region_trees[i]->set_region_to(test_data_values, i+1, test_data_shape);
	}
	print_array(test_data_values, test_data_shape);
	*/
	
	
	
	
	
	
	
	
	
}