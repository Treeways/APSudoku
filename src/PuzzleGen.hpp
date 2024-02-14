#pragma once

#include "Main.hpp"

namespace PuzzleGen
{
	struct PuzzleCell
	{
		u8 val;
		bool given;
		
		set<u8> options;
		void clear();
		PuzzleCell();
	};
	struct PuzzleGrid
	{
		PuzzleCell cells[9*9];
		PuzzleGrid();
		void print();
	private:
		void clear();
		pair<set<u8>,u8> trim_opts();
		void populate();
	};
	vector<pair<u8,bool>> gen_puzzle();
}

