#include "Main.hpp"
#include "PuzzleGen.hpp"

template<typename T>
T* rand(set<T>& s)
{
	if(s.empty()) return nullptr;
	size_t indx = rand(s.size());
	auto it = s.begin();
	std::advance(it,indx);
	return const_cast<T*>(&*it);
}
namespace PuzzleGen
{

PuzzleCell::PuzzleCell()
	: val(0), given(true),
	options({1,2,3,4,5,6,7,8,9})
{}

void PuzzleCell::clear()
{
	*this = PuzzleCell();
}

PuzzleGrid::PuzzleGrid()
{
	clear();
	populate();
}
void PuzzleGrid::clear()
{
	for (PuzzleCell& c : cells)
		c.clear();
}
pair<set<u8>,u8> PuzzleGrid::trim_opts()
{
	set<u8> least_opts;
	u8 least_count = 9;
	for(u8 index = 0; index < 9*9; ++index)
	{
		PuzzleCell& cell = cells[index];
		if(cell.val)
		{
			cell.options.clear();
			continue; //skip filled cells
		}
		cell.options = {1,2,3,4,5,6,7,8,9};
		u8 col = index%9;
		u8 row = index/9;
		u8 box = 3*(row/3)+(col/3);
		set<u8> neighbors;
		for(u8 q = 0; q < 9; ++q)
		{
			neighbors.insert(9*q + col);
			neighbors.insert(9*row + q);
			neighbors.insert(9*(3*(box/3) + (q/3)) + (3*(box%3) + (q%3)));
		}
		for(u8 q : neighbors)
			cell.options.erase(cells[q].val);
		u8 sz = cell.options.size();
		if(sz < least_count)
		{
			least_opts.clear();
			least_count = sz;
		}
		if(sz == least_count)
			least_opts.insert(index);
	}
	return {least_opts, least_count};
}

void PuzzleGrid::populate()
{
	vector<u8> history;
	while(true)
	{
		auto [rem,cnt] = trim_opts();
		if(cnt == 0) //failure
		{
			if(history.size() < 10)
			{
				//Fully retry, can't salvage
				//log("Failed puzzle gen... retrying");
				clear();
				return populate();
			}
			else
			{
				log("Backtracking...");
				//Undo the last 10 moves and try again
				for(u8 q = 0; q < 10; ++q)
				{
					cells[history.back()].clear();
					history.pop_back();
				}
				continue;
			}
		}
		if(rem.empty()) //success
			break;
		u8 targ_cell = *rand(rem);
		history.push_back(targ_cell);
		PuzzleCell& c = cells[targ_cell];
		c.val = *rand(c.options);
	}
}

void PuzzleGrid::print()
{
	u8 ind = 0;
	for(PuzzleCell const& c : cells)
	{
		std::cout << u16(c.val);
		if(ind % 9 == 8)
			std::cout << std::endl;
		else
		{
			std::cout << " ";
			if(ind % 3 == 2)
				std::cout << "  ";
		}
		++ind;
	}
}

vector<pair<u8,bool>> gen_puzzle()
{
	PuzzleGrid puzzle;
	puzzle.print();
	vector<pair<u8,bool>> ret;
	for(PuzzleCell const& cell : puzzle.cells)
		ret.emplace_back(cell.val, cell.given);
	
	return ret;
}

}

