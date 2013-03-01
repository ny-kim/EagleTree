/*
 * ssd_visual_tracer.cpp
 *
 *  Created on: Jun 7, 2012
 *      Author: niv
 */

#include "../ssd.h"
using namespace ssd;

VisualTracer *VisualTracer::inst = NULL;

VisualTracer::VisualTracer()
  : trace(SSD_SIZE, std::vector<std::vector<char> >(PACKAGE_SIZE, std::vector<char>(0) ))
{}

VisualTracer::~VisualTracer() {}

void VisualTracer::init()
{
	if (inst != NULL) delete inst;
	inst = new VisualTracer();
}

VisualTracer *VisualTracer::get_instance()
{
	return inst;
}

vector<char> get_int_as_char_vector(int num) {
	char buffer [10];
	int n = sprintf (buffer, "%d", num);
	vector<char> vec(n);
	for (int i = 0; i < n; i++) {
		vec[i] = buffer[i];
	}
	return vec;
}

void mark_as_gc_or_app(bool gc, bool app, vector<vector<char> >& symbols) {
	if (app) {
		vector<char> gc_symbol(3);
		gc_symbol[0] = 'A';
		gc_symbol[1] = 'P';
		gc_symbol[2] = 'P';
		symbols.push_back(gc_symbol);
	}
	if (gc) {
		vector<char> gc_symbol(2);
		gc_symbol[0] = 'G';
		gc_symbol[1] = 'C';
		symbols.push_back(gc_symbol);
	}
}

void VisualTracer::register_completed_event(Event& event) {
	if (event.get_event_type() == TRIM) {
		return;
	}
	Address add = event.get_address();

	int i = event.get_current_time() - event.get_execution_time() - trace[add.package][add.die].size();
	write(add.package, add.die, ' ', i);

	event_type type = event.get_event_type();
	if (type == WRITE) {
		write(add.package, add.die, 't', 2 * BUS_CTRL_DELAY + BUS_DATA_DELAY);
		vector<vector<char> > symbols;
		vector<char> logical_address = get_int_as_char_vector(event.get_logical_address());
		symbols.push_back(logical_address);
		vector<char> wait_time = get_int_as_char_vector(floor(event.get_bus_wait_time()));
		symbols.push_back(wait_time);
		mark_as_gc_or_app(event.is_garbage_collection_op(), event.is_original_application_io(), symbols);
		write_with_id(add.package, add.die, 'w', PAGE_WRITE_DELAY - 1, symbols);
	} else if (type == READ_COMMAND) {
		write(add.package, add.die, 't', BUS_CTRL_DELAY);
		//write(add.package, add.die, 'r', PAGE_READ_DELAY - 1);
		vector<vector<char> > symbols;
		vector<char> logical_address = get_int_as_char_vector(event.get_logical_address());
		symbols.push_back(logical_address);
		vector<char> wait_time = get_int_as_char_vector(floor(event.get_bus_wait_time()));
		symbols.push_back(wait_time);
		if (event.is_flexible_read()) {
			vector<char> gc_symbol(2);
			gc_symbol[0] = 'F';
			gc_symbol[1] = 'X';
			symbols.push_back(gc_symbol);
		}
		mark_as_gc_or_app(event.is_garbage_collection_op(), event.is_original_application_io(), symbols);
		write_with_id(add.package, add.die, 'r', PAGE_READ_DELAY - 1, symbols);
	} else if (type == READ_TRANSFER) {
		//write(add.package, add.die, 't', BUS_CTRL_DELAY + BUS_DATA_DELAY - 1);
		/*vector<vector<char> > symbols;
		vector<char> logical_address = get_int_as_char_vector(event.get_application_io_id());
		symbols.push_back(logical_address);
		mark_as_gc_or_app(event.is_garbage_collection_op(), event.is_original_application_io(), symbols);
		write_with_id(add.package, add.die, 't', BUS_CTRL_DELAY + BUS_DATA_DELAY - 1, symbols);*/
		write(add.package, add.die, 't', BUS_CTRL_DELAY + BUS_DATA_DELAY - 1);
	} else if (type == ERASE) {
		write(add.package, add.die, 't', BUS_CTRL_DELAY);
		/*vector<vector<char> > symbols;
		vector<char> logical_address = get_int_as_char_vector(event.get_id());
		symbols.push_back(logical_address);
		write_with_id(add.package, add.die, 'e', BLOCK_ERASE_DELAY - 1, symbols);*/
		write(add.package, add.die, 'e', BLOCK_ERASE_DELAY - 1);
	} else if (type == COPY_BACK) {
		vector<vector<char> > symbols;
		vector<char> logical_address = get_int_as_char_vector(event.get_id());
		symbols.push_back(logical_address);
		if (event.is_garbage_collection_op()) {
			vector<char> gc_symbol(2);
			gc_symbol[0] = 'C';
			gc_symbol[1] = 'B';
			symbols.push_back(gc_symbol);
		}

		write(add.package, add.die, 't', BUS_CTRL_DELAY);
		write(add.package, add.die, 'r', PAGE_READ_DELAY);
		write(add.package, add.die, 't', BUS_CTRL_DELAY);
		write_with_id(add.package, add.die, 'w', PAGE_WRITE_DELAY - 1, symbols);
	}
	trace[add.package][add.die].push_back('|');
}


void VisualTracer::write(int package, int die, char symbol, int length) {
	for (int i = 0; i < length; i++) {
		trace[package][die].push_back(symbol);
	}
}



void VisualTracer::write_with_id(int package, int die, char symbol, int length, vector<vector<char> > symbols) {

	uint length_remaining = length - 1;

	trace[package][die].push_back(symbol);

	for (uint i = 0; i < symbols.size() && symbols[i].size() < length_remaining; i++) {
		for (uint j = 0; j < symbols[i].size(); j++) {
			trace[package][die].push_back(symbols[i][j]);
		}
		trace[package][die].push_back(symbol);
		length_remaining -= symbols[i].size() + 1;
	}

	for (uint i = 0; i < length_remaining; i++) {
		trace[package][die].push_back(symbol);
	}
}

void VisualTracer::print_horizontally(int last_how_many_characters) {
	int starting_point;
	if (last_how_many_characters == UNDEFINED) {
		starting_point = 0;
	} else {
		starting_point = trace[0][0].size() - last_how_many_characters;
	}
	printf("\n");
	for (uint i = 0; i < SSD_SIZE; i++) {
		for (uint j = 0; j < PACKAGE_SIZE; j++) {
			printf("p%d d%d :", i, j);
			for (uint c = starting_point; c < trace[i][j].size(); c++) {
				printf("%c", trace[i][j][c]);
			}
			printf("\n");
		}
	}
}


void VisualTracer::print_horizontally_with_breaks(long cursor) {
	printf("\n");
	int chars_to_write_each_time = 150;
	while (cursor < trace[0][0].size()) {
		for (uint i = 0; i < SSD_SIZE; i++) {
			for (uint j = 0; j < PACKAGE_SIZE; j++) {
				printf("p%d d%d :", i, j);
				for (uint c = 0; c < chars_to_write_each_time; c++) {
					if (trace[i][j].size() > cursor + c) {
						printf("%c", trace[i][j][cursor + c]);
					}
				}
				printf("\n");
			}
		}
		printf("\nLine %d\n", cursor / chars_to_write_each_time);
		cursor += chars_to_write_each_time;
	}
}

void VisualTracer::print_horizontally_with_breaks_last(long how_many_chars) {
	print_horizontally_with_breaks(trace[0][0].size() - how_many_chars);
}

void VisualTracer::print_vertically() {
	printf("\n");
	for (uint c = 0; c < trace[0][0].size(); c++) {
		for (uint i = 0; i < SSD_SIZE; i++) {
			for (uint j = 0; j < PACKAGE_SIZE; j++) {
				if (trace[i][j].size() > c) {
					printf("%c", trace[i][j][c]);
				}
			}
		}
		printf("\n");
	}

}

