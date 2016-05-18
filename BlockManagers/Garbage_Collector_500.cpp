#include "../ssd.h"
#include "../block_management.h"
using namespace ssd;

int SUPERBLOCK_SIZE = 1<<2;
int check_on_flag=1;

unsigned long previous_erased_addr=0;
unsigned long previous_invalidated_addr=0;

//vector<page_num, set<long> >

Garbage_Collector_500::Garbage_Collector_500()
:  Garbage_Collector(),
   gc_candidates(SSD_SIZE, vector<vector<set<long> > > (PACKAGE_SIZE, vector<set<long> >(SUPERBLOCK_SIZE, set<long>()))),
   superblock_pe(SSD_SIZE, vector<vector<long > > (PACKAGE_SIZE, vector<long >(SUPERBLOCK_SIZE, 0))),
   superblock_invalid(SSD_SIZE, vector<vector<long > > (PACKAGE_SIZE, vector<long >(SUPERBLOCK_SIZE, 0)))


//gc_candidates(SSD_SIZE, vector<set<long> >(PACKAGE_SIZE, set<long>()))
{}

Garbage_Collector_500::Garbage_Collector_500(Ssd* ssd, Block_manager_parent* bm)
:  Garbage_Collector(ssd, bm),
   gc_candidates(SSD_SIZE, vector<vector<set<long> > > (PACKAGE_SIZE, vector<set<long> >(SUPERBLOCK_SIZE, set<long>()))),
   superblock_pe(SSD_SIZE, vector<vector<long > > (PACKAGE_SIZE, vector<long >(SUPERBLOCK_SIZE, 0))),
   superblock_invalid(SSD_SIZE, vector<vector<long > > (PACKAGE_SIZE, vector<long >(SUPERBLOCK_SIZE, 0)))
//gc_candidates(SSD_SIZE, vector<set<long> >(PACKAGE_SIZE, set<long>()))

{}


uint Garbage_Collector_500::get_superblock_idx(Address const& phys_address){
//	SSD_SIZE = 4;
//		PACKAGE_SIZE = 2;
//		DIE_SIZE = 1;
//		PLANE_SIZE = 1024;
//		BLOCK_SIZE = 128;
//	return phys_address.block/(PLANE_SIZE/SUPERBLOCK_SIZE);
	return phys_address.block>>8;
}




void Garbage_Collector_500::commit_choice_of_victim(Address const& phys_address, double time) {
	uint superblock_idx = get_superblock_idx(phys_address);
	gc_candidates[phys_address.package][phys_address.die][superblock_idx].erase(phys_address.get_linear_address());

	if( superblock_pe[phys_address.package][phys_address.die][superblock_idx]  ){
		//compare and update superblock_pe if new pe is bigger than
	}
}



vector<long> Garbage_Collector_500::get_relevant_gc_candidates(int package_id, int die_id, int klass) const {
	vector<long > candidates;
	int package = package_id == -1 ? 0 : package_id;
	int num_packages = package_id == -1 ? SSD_SIZE : package_id + 1;
	for (; package < num_packages; package++) {
		int die = die_id == -1 ? 0 : die_id;
		int num_dies = die_id == -1 ? PACKAGE_SIZE : die_id + 1;
		for (; die < num_dies; die++) {

			//  select the best superblock
			int best_superblock = 0;
			int best_score = superblock_pe[package][die][best_superblock]+superblock_invalid[package][die][best_superblock];
			for(int superblock_i=0;superblock_i<SUPERBLOCK_SIZE;superblock_i++){
				if(best_score<superblock_pe[package][die][superblock_i]+superblock_invalid[package][die][superblock_i]){
					best_superblock=superblock_i;
					best_score=superblock_pe[package][die][superblock_i]+superblock_invalid[package][die][superblock_i];

				}

			}

			for (auto i : gc_candidates[package][die][best_superblock]) {
				candidates.push_back(i);
			}
		}
	}
	return candidates;
}

Block* Garbage_Collector_500::choose_gc_victim(int package_id, int die_id, int klass) const {
	vector<long> candidates = get_relevant_gc_candidates(package_id, die_id, klass);
	uint min_valid_pages = BLOCK_SIZE;
	Block* best_block = NULL;
	Address best_address = NULL;
	for (auto physical_address : candidates) {
		Address a = Address(physical_address, BLOCK);
		Block* block = ssd->get_package(a.package)->get_die(a.die)->get_plane(a.plane)->get_block(a.block);

		if (block->get_pages_valid() < min_valid_pages && (block->get_state() == ACTIVE || block->get_state() == INACTIVE)) {
			min_valid_pages = block->get_pages_valid();
			best_block = block;
			best_address = a;
			assert(min_valid_pages < BLOCK_SIZE);
		}
	}

	//update...

	return best_block;
}

void Garbage_Collector_500::invalidate_event_completion(Event & event) {
	Address ra = event.get_replace_address();
	if(true || event.invalidate_page_flag && previous_invalidated_addr != ra.get_linear_address() ){
		previous_invalidated_addr=ra.get_linear_address();
		uint superblock_idx = get_superblock_idx(ra);
		superblock_invalid[ra.package][ra.die][superblock_idx]++;
		event.invalidate_page_flag=0;
	}
}

void Garbage_Collector_500::erase_event_completion(Event & event) {
	Address ra = event.get_replace_address();

	if(true || event.erased_invalid>0  /*&& previous_erased_addr != ra.get_linear_address() */){

		previous_erased_addr=ra.get_linear_address();
		printf("hi\n");

		Block& block = *ssd -> get_package(ra.package)->get_die(ra.die)->get_plane(ra.plane)->get_block(ra.block);
		uint superblock_idx = get_superblock_idx(ra);

		int minus_invalid=event.erased_invalid;
		event.erased_invalid=0;

		if(ra.package==0 && ra.die==0 && superblock_idx==1){
			printf("\t\t!!! %d %d\n", superblock_invalid[ra.package][ra.die][superblock_idx], minus_invalid);
		}
		superblock_invalid[ra.package][ra.die][superblock_idx]-=minus_invalid;
//		if(block.get_age()>0){
//			printf("erase try erase try erase try erase try !! %d %d\n", superblock_pe[ra.package][ra.die][superblock_idx], block.get_age());
//		}

		if(superblock_pe[ra.package][ra.die][superblock_idx]<block.get_age()){
			superblock_pe[ra.package][ra.die][superblock_idx]=block.get_age();
			superblock_max_pe = (superblock_max_pe>block.get_age()) ?  superblock_max_pe : block.get_age();

//			printf("erase!! %d\n", superblock_pe[ra.package][ra.die][superblock_idx]);
		}
	}
}

void Garbage_Collector_500::register_event_completion(Event & event) {
	Address ra = event.get_replace_address();

	if (event.get_event_type() != WRITE) {
		return;
	}

	if(event.invalidate_page_flag /*&& previous_invalidated_addr != ra.get_linear_address() */){
		previous_invalidated_addr=ra.get_linear_address();
		uint superblock_idx = get_superblock_idx(ra);
		superblock_invalid[ra.package][ra.die][superblock_idx]++;
		event.invalidate_page_flag=0;
	}

	if (ra.valid == NONE) {
		return;
	}
	ra.valid = BLOCK;
	ra.page = 0;

	//sim edit
	Block& block = *ssd -> get_package(ra.package)->get_die(ra.die)->get_plane(ra.plane)->get_block(ra.block);

	//if(block.get_age()>0){
	//			printf("Inserting as GC candidate: %ld ", ra.get_linear_address());
	//					ra.print();
	//			printf(" with age_class %d and valid blocks: %d-%d\n", block.get_age(), block.get_pages_valid(), block.get_pages_invalid());
	//}
	// sim edit end

	uint superblock_idx = get_superblock_idx(ra);
	if(check_on_flag==1){
		printf("I am 500\n");
		check_on_flag++;
	}
	gc_candidates[ra.package][ra.die][superblock_idx].insert(ra.get_linear_address());
	if (gc_candidates[ra.package][ra.die][superblock_idx].size() == 1) {
		bm->check_if_should_trigger_more_GC(event);
	}
}
