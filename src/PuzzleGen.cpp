#include "Main.hpp"
#include "GUI.hpp"
#include "PuzzleGen.hpp"
#include <thread>

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

void PuzzleCell::reset_opts()
{
	options = {1,2,3,4,5,6,7,8,9};
}
void PuzzleCell::clear()
{
	*this = PuzzleCell();
}

struct GridFillHistory
{
	u8 ind;
	map<u8,set<u8>> checked;
	GridFillHistory() : ind(0), checked() {}
};
struct GridGivenHistory
{
	u8 ind;
	set<u8> checked;
	GridGivenHistory() : ind(0), checked() {}
};

PuzzleGrid::PuzzleGrid(Difficulty d)
	: PuzzleGrid()
{
	populate();
	build(d);
}
PuzzleGrid::PuzzleGrid()
{
	clear();
}
void PuzzleGrid::clear()
{
	for (PuzzleCell& c : cells)
		c.clear();
}
PuzzleGrid PuzzleGrid::given_copy(PuzzleGrid const& g)
{
	PuzzleGrid ret;
	for(u8 q = 0; q < 9*9; ++q)
	{
		ret.cells[q].given = g.cells[q].given;
		if(ret.cells[q].given)
			ret.cells[q].val = g.cells[q].val;
	}
	return ret;
}
bool PuzzleGrid::is_unique() const
{
	PuzzleGrid test = given_copy(*this);
	return test.solve(true);
}

pair<set<u8>,u8> PuzzleGrid::trim_opts(map<u8,set<u8>> const& banned)
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
		auto banset = banned.find(index);
		if(banset != banned.end())
			for(u8 val : banset->second)
				cell.options.erase(val);
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
		u8 sz = (u8)cell.options.size();
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

bool PuzzleGrid::solve(bool check_unique)
{
	// if `check_unique` is true, the puzzle will be mangled,
	//     but the function will return if it has a unique solution.
	// else, the puzzle will be solved with a unique solution, returning success.
	for (PuzzleCell& c : cells)
		c.reset_opts();
	bool solved = false;
	vector<GridFillHistory> history;
	history.emplace_back(); //add first step
	while(true)
	{
		GridFillHistory& step = history.back();
		// Trim the options, accounting for anything we've already failed trying
		auto [rem,cnt] = trim_opts(step.checked);
		bool goback = cnt == 0;
		if(rem.empty() && !goback) //success
		{
			if(check_unique)
			{
				if(solved)
					return false;
				solved = true;
				goback = true;
			}
			else return true;
		}
		if(goback) //failure
		{
			// Step back, and mark this as a bad path
			history.pop_back();
			if(history.empty())
				break;
			GridFillHistory& prev = history.back();
			cells[prev.ind].clear();
			continue;
		}
		// continuing
		// Assign a random least-options cell to a random of its options
		step.ind = *rand(rem);
		PuzzleCell& c = cells[step.ind];
		c.val = *rand(c.options);
		step.checked[step.ind].insert(c.val);
		history.emplace_back(); //add the next step
	}
	return solved;
}

//Fills the grid with a random valid solution
void PuzzleGrid::populate()
{
	clear();
	solve(false);
}
//Starting from a filled grid, trims away givens
void PuzzleGrid::build(Difficulty d)
{
	//Grid should be filled with a valid end solution before call
	set<u8> givens;
	for(u8 q = 0; q < 9*9; ++q)
	{
		if(!cells[q].given)
			throw puzzle_gen_exception("non-full grid cannot be built");
		else givens.insert(q);
	}
	//
	u8 target_givens;
	switch(d)
	{
		case DIFF_EASY:
			target_givens = 48;
			break;
		case DIFF_NORMAL:
			target_givens = 35;
			break;
		case DIFF_HARD:
			target_givens = 24;
			break;
	}
	vector<GridGivenHistory> history;
	history.emplace_back();
	while(true)
	{
		GridGivenHistory& step = history.back();
		set<u8> possible = givens;
		for(u8 q : step.checked)
			possible.erase(q);
		if(possible.empty()) //fail, need backtrack
		{
			history.pop_back();
			if(history.empty())
				break;
			GridGivenHistory& prev = history.back();
			cells[prev.ind].given = true;
			continue;
		}
		step.ind = *rand(possible);
		cells[step.ind].given = false;
		step.checked.insert(step.ind);
		if(!is_unique()) //fail, retry this step
		{
			cells[step.ind].given = true;
			continue;
		}
		givens.erase(step.ind);
		if(givens.size() == target_givens)
			return; //success!
		history.emplace_back();
	}
	throw puzzle_gen_exception("grid build error");
}



void PuzzleGrid::print() const
{
	u8 ind = 0;
	for(PuzzleCell const& c : cells)
	{
		std::cout << (c.given ? u16(c.val) : 0);
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

void PuzzleGrid::print_sol() const
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

static void _puzzle_loadscr(volatile bool* running)
{
	optional<u8> _ret;
	bool _foo;
	Dialog popup;
	popups.emplace_back(&popup);
	generate_popup(popup, _ret, _foo, "Please Wait", "Generating puzzle...", {});
	popup.run_proc = [&running]()
		{
			return *running;
		};
	popup.run_loop();
	popups.pop_back();
}
vector<pair<u8,bool>> gen_puzzle(Difficulty d)
{
	volatile bool running = true;
	std::thread popup_thread(_puzzle_loadscr, &running);
	
	PuzzleGrid puzzle(d);
	vector<pair<u8,bool>> ret;
	for(PuzzleCell const& cell : puzzle.cells)
		ret.emplace_back(cell.val, cell.given);
	
	running = false;
	popup_thread.join();
	
	return ret;
}

}

