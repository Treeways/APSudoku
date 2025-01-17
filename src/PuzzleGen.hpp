#pragma once

#include "Main.hpp"

namespace PuzzleGen
{
	void init();
	void shutdown();
	
	typedef vector<pair<u8,bool>> BuiltPuzzle;
	struct PuzzleCell
	{
		u8 val;
		bool given;
		
		set<u8> options;
		void clear();
		void reset_opts();
		PuzzleCell();
	};
	struct PuzzleGrid
	{
		PuzzleCell cells[9*9];
		PuzzleGrid(Difficulty d);
		
		static PuzzleGrid given_copy(PuzzleGrid const& g);
		bool is_unique() const;
		void print() const;
		void print_sol() const;
	private:
		PuzzleGrid();
		void clear();
		
		pair<set<u8>,u8> trim_opts(map<u8,set<u8>> const& banned);
		bool solve(bool check_unique);
		void populate();
		void build(Difficulty d);
	};
	BuiltPuzzle gen_puzzle(Difficulty d);
	
	class puzzle_gen_exception : public sudoku_exception
	{
	public:
		puzzle_gen_exception(string const& msg)
			: sudoku_exception(msg)
		{}
	};
}

