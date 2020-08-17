#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <map>


// program to protoype difference upload system

struct CutListEntryBasic 
{
    uint32_t skeletonNodeId = UINT_MAX;
    uint32_t skeletonParentNodeId = UINT_MAX;
};

template<typename T>
inline std::ostream & operator<<(std::ostream & os, std::vector<T> vec)
{
    // os<<"{ ";
    std::copy(vec.begin(), vec.end(), std::ostream_iterator<T>(os, ","));
    // os<<"}";
    return os;
}

int main() {
    std::cout << "" << std::endl;


    const uint32_t fullTimeSequenceLength = 12;
	const uint32_t max_cut_list_size = 50;
	const uint32_t min_cut_list_size = 40;

    // create mock cut lists
    std::vector< std::vector<CutListEntryBasic> >  cut_lists   (fullTimeSequenceLength, std::vector<CutListEntryBasic>(max_cut_list_size));
    std::vector<uint32_t> 						   frameBlocks (fullTimeSequenceLength);

    // outputs of diff prep process
    std::vector< std::vector<uint32_t> >  reordered_cut_lists   (fullTimeSequenceLength, std::vector<uint32_t>(max_cut_list_size));

    std::vector<uint32_t> 	everPresentBlocks (fullTimeSequenceLength);
    std::vector<uint32_t> 	newBlocks         (fullTimeSequenceLength);


    // create vector of possible svo ids to shuffle for each time step
    const uint32_t svo_id_max = max_cut_list_size*2;
    std::vector<uint32_t> numbers;
    for(int i=0; i<svo_id_max; i++) numbers.push_back(i);

    // fill each with a variable length list of unique svo skeleton indices
    for (uint32_t i = 0; i < fullTimeSequenceLength; ++i)
    {
    	const uint32_t cut_list_length = min_cut_list_size + (rand() % (max_cut_list_size - min_cut_list_size) );
	    frameBlocks[i] = cut_list_length;

	    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	    std::shuffle(numbers.begin(), numbers.end(), std::default_random_engine(seed));

	    for (int j = 0; j < cut_list_length; ++j)
	    {
	    	cut_lists[i][j].skeletonNodeId = numbers[j];
	    	cut_lists[i][j].skeletonParentNodeId = numbers[j]/2;

	    	// print cut lists
	    	// std::cout << cut_lists[i][j].skeletonNodeId << ", ";
	    }
	    std::cout << std::endl;

    }


    // subsequences are determined by an arbitrary subsequence length

    const uint32_t max_subseq_length = 5;

    for (uint32_t start_f = 0; start_f < fullTimeSequenceLength; start_f += max_subseq_length)
    {
    	const uint32_t end_f =std::min(start_f + max_subseq_length, fullTimeSequenceLength);
    	const uint32_t subseq_length = end_f - start_f;
    	// print subsequence start and end
    	std::cout << start_f << " - " << end_f << std::endl;



    	// create vector with all svo indices in the subsequence
    	std::vector<uint32_t> svo_indices ( std::accumulate(frameBlocks.begin() + start_f, frameBlocks.begin() + end_f,0), UINT_MAX);
    	uint32_t write_idx = 0;
    	for (uint32_t f = start_f; f < end_f; ++f){
    		for (int i = 0; i < frameBlocks[f]; ++i){

    			//TODO could save a map from skeleton nodes to parent nodes at this point
    			// for later reconstruction of cut lists

    			svo_indices[write_idx++] = cut_lists[f][i].skeletonNodeId;
    		}
    	}

    	// remove duplicates
    	std::sort(svo_indices.begin(), svo_indices.end());
    	const auto it = std::unique(svo_indices.begin(), svo_indices.end());
    	svo_indices.resize(std::distance(svo_indices.begin(), it));

		std::cout << "Unique indices: " << svo_indices.size() << std::endl;

    	// create map from svo skeleton id to presence in subsequence and initialise all entries with 0
		std::map<uint32_t,  uint32_t> svo_idx_to_presence;
		for (const auto idx : svo_indices) svo_idx_to_presence[idx] = 0;

		// process all present ids and build presence flags
    	for (uint32_t f = start_f; f < end_f; ++f){
    		for (uint32_t i = 0; i < frameBlocks[f]; ++i){
    			svo_idx_to_presence[ cut_lists[f][i].skeletonNodeId ] |= (1 << (f-start_f) );
    			// svo_idx_to_presence[ cut_lists[f][i].skeletonNodeId ]++;
    		}
		}

		// //check flags
		// for (auto itr = svo_idx_to_presence.begin(); itr != svo_idx_to_presence.end(); ++itr) { 
		//         std::cout << '\t' << itr->first 
		//              << '\t' << itr->second << '\n'; 
		//     } 

		// search map to find and remove ever-present nodes
		const uint32_t ever_present_flag = pow(2,subseq_length-1);
		std::vector<uint32_t> ever_presents;
		for(auto it = svo_idx_to_presence.begin(); it != svo_idx_to_presence.end(); ){
			if (it->second == ever_present_flag ) {
				ever_presents.push_back(it->first);
				it = svo_idx_to_presence.erase(it);
			}
			else {
				++it;
			}
		}

		std::cout << "Ever presents: " << ever_presents << std::endl; 

		// sort and save ever present blocks
		std::sort(ever_presents.begin(), ever_presents.end());
    	for (uint32_t f = start_f; f < end_f; ++f){
    		everPresentBlocks[f] = ever_presents.size();
    		std::copy(ever_presents.begin(), ever_presents.end(), reordered_cut_lists[f].begin());
    	}

    	// TODO : separate list for new blocks?
   

  //   	uint32_t cut_list_write = ever_presents.size();

  //   	// for first frame, all blocks are new and can be added to new blocks list
  //   	// avoids checking against non existent previous frame
		// for(auto it = svo_idx_to_presence.begin(); it != svo_idx_to_presence.end(); ++it){
		// 	if (it->second & 1) {
		// 		newBlocks[start_f][new_b_write] == it->first;
		// 	}
		// }


  //   	// find differences and new blocks for each frame
  //   	for (uint32_t f = start_f+1; f < end_f; ++f){

  //   		// find those not previously present
  //   		// filter map to find those present in active frame
  //   		//check if they were previously added
  //   		if 

  //   		// add to cut list, appending to previously observed 

  //   		// for those previously observed, add to frame update list
  //   		// also save cut list id for update

		// }





    }





    return 0;
}