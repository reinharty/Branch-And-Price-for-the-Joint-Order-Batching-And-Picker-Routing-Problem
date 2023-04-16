#include "warehouse.h"

#include <iostream>
#include <cmath>
#include <map>
#include <set>
#include <cassert>
#include <random>
#include <algorithm>

c_SKU_Collection::c_SKU_Collection(const std::vector<c_SKU>& all_SKUs, bool selection)
	: v_all_SKUs(all_SKUs.begin(), all_SKUs.end()),
	i_largest_article_id(0),
	i_first_aisle_with_sku(-1),
	i_last_aisle_with_sku(-1),
	b_scattered_storage(false),
	b_selection(selection)
{
	if (v_all_SKUs.empty())
	{
		std::cerr << "Empty vector of SKUs not allowed." << std::endl;
		throw;
	}
	// sort by article
	sort(v_all_SKUs.begin(), v_all_SKUs.end());
	std::set<int> articles_set;
	for (auto sku : v_all_SKUs)
		articles_set.insert(sku.Article());
	v_all_articles = std::vector<int>(articles_set.begin(), articles_set.end());
	i_largest_article_id = v_all_articles.back();
	v_SKU_of_article.resize(i_largest_article_id + 1);
	int pred = -1;
	for (auto sku : v_all_SKUs)
	{
		if (pred == sku.Article())
			b_scattered_storage = true;
		pred = sku.Article();
	}
	v_first_aisle_with_article.resize(i_largest_article_id + 1, std::numeric_limits<int>::max());
	v_last_aisle_with_article.resize(i_largest_article_id + 1, 0);
	for (auto sku : v_all_SKUs)
	{
		int a = sku.Article();
		v_SKU_of_article[a].push_back(sku);
		if (v_last_aisle_with_article[a] < sku.Aisle())
			v_last_aisle_with_article[a] = sku.Aisle();
		if (v_first_aisle_with_article[a] > sku.Aisle())
			v_first_aisle_with_article[a] = sku.Aisle();
	}
	// sort by aisle and cell (tie break: article)
	sort(v_all_SKUs.begin(), v_all_SKUs.end(),
		[](const c_SKU& first, const c_SKU& second) -> bool
	{
		if (first.Aisle() < second.Aisle())
			return true;
		if (first.Aisle() == second.Aisle())
		{
			if (first.Cell() < second.Cell())
				return true;
			if (first.Cell() == second.Cell())
			{
				if (first.Article() < second.Article())
					return true;
			}
		}
		return false;
	});
	int last_aisle = 0;
	for (auto sku : v_all_SKUs)
		last_aisle = std::max(last_aisle, sku.Aisle());
	v_SKU_by_aisle.resize(last_aisle + 1);
	for (auto sku : v_all_SKUs)
		v_SKU_by_aisle[sku.Aisle()].push_back(sku);
	// determine first and last aisle
	for (int aisle = 0; aisle < (int)v_SKU_by_aisle.size(); aisle++)
	{
		if (!v_SKU_by_aisle[aisle].empty())
		{
			if (i_first_aisle_with_sku == -1)
				i_first_aisle_with_sku = aisle;
			i_last_aisle_with_sku = aisle;
		}
	}
	// determine non-empty cells per aisle
	v_non_empty_cells_in_aisle.resize(last_aisle + 1);
	for (int aisle = 0; aisle < (int)v_non_empty_cells_in_aisle.size(); aisle++)
	{
		std::set<int> cells;
		for (auto sku : v_SKU_by_aisle[aisle])
			cells.insert(sku.Cell());
		v_non_empty_cells_in_aisle[aisle].assign(cells.begin(), cells.end());
	}
	//articles in aisle sorted by position
	v_articles_in_aisle_sorted_by_pos.resize(last_aisle + 1);
	for (auto sku : v_all_SKUs)
		v_articles_in_aisle_sorted_by_pos[sku.Aisle()].push_back(sku.Article());
	// determine articles per aisle and pos
	v_articles_in_aisle_at_pos.resize(last_aisle + 1);
	v_articles_in_aisle_between_top_and_pos.resize(last_aisle + 1);
	v_articles_in_aisle_between_pos_and_bottom.resize(last_aisle + 1);
	for (int aisle = 0; aisle < last_aisle + 1; aisle++)
	{
		int positions_in_aisle = (int)v_non_empty_cells_in_aisle[aisle].size();
		v_articles_in_aisle_at_pos[aisle].resize(positions_in_aisle);
		v_articles_in_aisle_between_top_and_pos[aisle].resize(positions_in_aisle);
		v_articles_in_aisle_between_pos_and_bottom[aisle].resize(positions_in_aisle);
		for (auto sku : v_SKU_by_aisle[aisle])
		{
			int cell = sku.Cell();
			int pos = -1;
			for (int p = 0; p < positions_in_aisle; p++)
			{
				if (cell == v_non_empty_cells_in_aisle[aisle][p])
				{
					pos = p;
					break;
				}
			}
			assert(pos >= 0);
			for (int p = 0; p < positions_in_aisle; p++)
			{
				if (p == pos)
					v_articles_in_aisle_at_pos[aisle][p].push_back(sku.Article());
				if (p >= pos)
					v_articles_in_aisle_between_top_and_pos[aisle][p].push_back(sku.Article());
				if (p <= pos)
					v_articles_in_aisle_between_pos_and_bottom[aisle][p].push_back(sku.Article());
			}
		}
		// there may be dublicates, we eliminate them
		for (int p = 0; p < positions_in_aisle; p++)
		{
			std::set<int> set_a(v_articles_in_aisle_between_top_and_pos[aisle][p].begin(), v_articles_in_aisle_between_top_and_pos[aisle][p].end());
			v_articles_in_aisle_between_top_and_pos[aisle][p].assign(set_a.begin(), set_a.end());

			std::set<int> set_b(v_articles_in_aisle_between_pos_and_bottom[aisle][p].begin(), v_articles_in_aisle_between_pos_and_bottom[aisle][p].end());
			v_articles_in_aisle_between_pos_and_bottom[aisle][p].assign(set_b.begin(), set_b.end());
		}
	}
}

// articles

const std::vector<c_SKU>& c_SKU_Collection::SKUs() const
{
	return v_all_SKUs;
}

const std::vector<int>& c_SKU_Collection::AllArticles() const
{
	return v_all_articles;
}

int c_SKU_Collection::LargestArticleID() const
{
	return i_largest_article_id;
}

static const std::vector<c_SKU> empty_skus;

const std::vector<c_SKU>& c_SKU_Collection::SKUsInAisle(int aisle) const
{
	if (aisle < v_SKU_by_aisle.size())
		return v_SKU_by_aisle[aisle];
	else
		return empty_skus;
}

int c_SKU_Collection::FirstAisleWithSKU() const
{
	return i_first_aisle_with_sku;
}

int c_SKU_Collection::LastAisleWithSKU() const
{
	return i_last_aisle_with_sku;
}

bool c_SKU_Collection::IsAisleEmpty(int aisle) const
{
	if (aisle < v_SKU_by_aisle.size())
		return v_SKU_by_aisle[aisle].empty();
	else
		return true;
}

int c_SKU_Collection::FirstAisleWithArticle(int a) const
{
	return v_first_aisle_with_article[a];
}

int c_SKU_Collection::LastAisleWithArticle(int a) const
{
	return v_last_aisle_with_article[a];
}

int c_SKU_Collection::NumberOfNonEmptyCellsInAisle(int aisle) const
{
	if (aisle < v_non_empty_cells_in_aisle.size())
		return (int)v_non_empty_cells_in_aisle[aisle].size();
	else
		return 0;
}

static const std::vector<int> empty_vec_int;

const std::vector<int>& c_SKU_Collection::NonEmptyCellsInAisle(int aisle) const
{
	if (aisle < v_non_empty_cells_in_aisle.size())
		return v_non_empty_cells_in_aisle[aisle];
	else
		return empty_vec_int;
}

const std::vector<int>& c_SKU_Collection::ArticlesInAisle(int aisle) const
{
	if (aisle >= v_articles_in_aisle_between_pos_and_bottom.size())
		return empty_vec_int;
	if (v_articles_in_aisle_between_pos_and_bottom[aisle].empty())
		return empty_vec_int;
	return v_articles_in_aisle_between_pos_and_bottom[aisle][0];
}

const std::vector<int>& c_SKU_Collection::ArticlesInAisleBetweenTopAndPos(int aisle, int pos) const
{
	return v_articles_in_aisle_between_top_and_pos[aisle][pos];
}


const std::vector<int>& c_SKU_Collection::ArticlesInAisleBetweenPosAndBottom(int aisle, int pos) const
{
	return v_articles_in_aisle_between_pos_and_bottom[aisle][pos];
}

const std::vector<int>& c_SKU_Collection::ArticlesInAisleSortedByPos(int aisle) const
{
	return v_articles_in_aisle_sorted_by_pos[aisle];
}

std::vector<int> c_SKU_Collection::ArticlesInAisleBetweenTopAndCell(int aisle, int cell) const
{
	std::vector<int> articles;
	for (auto it = v_SKU_by_aisle[aisle].begin(); it != v_SKU_by_aisle[aisle].end(); it++)
	{
		if ((*it).Cell() > cell)
			break;
		articles.push_back((*it).Article());
	}
	return articles;
}


std::vector<int> c_SKU_Collection::ArticlesInAisleBetweenCellAndBottom(int aisle, int cell) const
{
	std::vector<int> articles;
	for (auto it = v_SKU_by_aisle[aisle].rbegin(); it != v_SKU_by_aisle[aisle].rend(); it++)
	{
		if ((*it).Cell() < cell)
			break;
		articles.push_back((*it).Article());
	}
	reverse(articles.begin(), articles.end());
	return articles;
}

const bool c_SKU_Collection::PositionIsRedundant(int aisle, int pos) const
{
	return PositionsAreRedundant(aisle, pos, pos);
}


const bool c_SKU_Collection::PositionsAreRedundant(int aisle, int pos1, int pos2) const
{
	if (b_selection)
		return true;
	// more convenient to call the function with pos2=-1 for the last cell
	if (pos2 == -1)
		pos2 = NumberOfNonEmptyCellsInAisle(aisle) - 1;
	if (pos1 > pos2)
		return true;
	auto all_cells = NonEmptyCellsInAisle(aisle);
	int cell1 = all_cells[pos1];
	int cell2 = all_cells[pos2];
	for (auto a : ArticlesInAisle(aisle))
	{
		int demand = 1;// TODO to be replaced by true demand later
		int supply = 0;
		for (auto sku : SKUsInAisle(aisle))
			if (sku.Cell() >= cell1 && sku.Cell() <= cell2 && sku.Article() == a)
			{
				supply += sku.Quantity();
			}
		if (TotalQuantity(a) - supply < demand)
			return false;
	}
	return true;
}

const std::vector<c_SKU>& c_SKU_Collection::SKUsOfArticle(int a) const
{
	return v_SKU_of_article[a];
}


bool c_SKU_Collection::ArticleHasUniquePosition(int a) const
{
	return (v_SKU_of_article[a].size() == 1);
}


int c_SKU_Collection::TotalQuantity(int a) const
{
	int ret = 0;
	for (auto sku : v_SKU_of_article[a])
		ret += sku.Quantity();
	return ret;
}


bool c_SKU_Collection::ArticleIsScattered(int a) const
{
	return !ArticleHasUniquePosition(a);
}


bool c_SKU_Collection::ScatteredStorage() const
{
	return b_scattered_storage;
}

bool c_SKU_Collection::WithOptions() const
{
	return ScatteredStorage() || b_selection;
}

// class c_WarehouseLayout

c_WarehouseLayout::c_WarehouseLayout(const std::vector<int> aisle_to_aisle_dist, const std::vector<int> cell_to_cell_dist, int dist_top_to_last_cell, int dist_bottom_to_first_cell, bool depot_at_bottom, int depot_aisle, int dist_top_or_bottom_to_depot)
	: v_aisle_to_aisle_dist(aisle_to_aisle_dist),
	v_cell_to_cell_dist(cell_to_cell_dist),
	i_dist_top_to_last_cell(dist_top_to_last_cell),
	i_dist_bottom_to_first_cell(dist_bottom_to_first_cell),
	b_depot_at_bottom(depot_at_bottom),
	i_depot_aisle(depot_aisle),
	i_dist_top_or_bottom_to_depot(dist_top_or_bottom_to_depot)
{
	// aisle to aisle
	v_cumulative_aisle_to_aisle_dist.push_back(0);
	for (auto dist : v_aisle_to_aisle_dist)
		v_cumulative_aisle_to_aisle_dist.push_back(dist + v_cumulative_aisle_to_aisle_dist.back());
	// aisle to aisle
	v_cumulative_cell_to_cell_dist.push_back(0);
	for (auto dist : v_cell_to_cell_dist)
		v_cumulative_cell_to_cell_dist.push_back(dist + v_cumulative_cell_to_cell_dist.back());
}

int c_WarehouseLayout::NumberOfAisles() const
{
	return (int)v_cumulative_aisle_to_aisle_dist.size();
}

int c_WarehouseLayout::NumberOfCellsPerAisle() const
{
	return (int)v_cumulative_cell_to_cell_dist.size();
}

bool c_WarehouseLayout::DepotIsAtTop() const
{
	return !DepotIsAtBottom();
}

bool c_WarehouseLayout::DepotIsAtBottom() const
{
	return b_depot_at_bottom;
}

int c_WarehouseLayout::AisleOfDepot() const
{
	return i_depot_aisle;
}

int c_WarehouseLayout::DistanceTopToBottom() const
{
	return i_dist_top_to_last_cell + v_cumulative_cell_to_cell_dist.back() + i_dist_bottom_to_first_cell;
}

int c_WarehouseLayout::DistanceAisleToAisle(int i, int j) const
{
	return abs(v_cumulative_aisle_to_aisle_dist[i] - v_cumulative_aisle_to_aisle_dist[j]);
}

int c_WarehouseLayout::DistanceCellToCell(int i, int j) const
{
	return abs(v_cumulative_cell_to_cell_dist[i] - v_cumulative_cell_to_cell_dist[j]);
}

int c_WarehouseLayout::DistanceTopToCell(int i) const
{
	return i_dist_top_to_last_cell + v_cumulative_cell_to_cell_dist[i];
}

int c_WarehouseLayout::DistanceCellToBottom(int i) const
{
	return i_dist_bottom_to_first_cell + v_cumulative_cell_to_cell_dist.back() - v_cumulative_cell_to_cell_dist[i];
}

int c_WarehouseLayout::DistancePosToPos(c_Position pos1, c_Position pos2) const
{
	int aisle1 = pos1.Aisle();
	int aisle2 = pos2.Aisle();
	int cell1 = pos1.Cell();
	int cell2 = pos2.Cell();

	if (aisle1 == aisle2)
		return DistanceCellToCell(cell1, cell2); // same aisle
												 // otherwise different aisles
	return DistanceAisleToAisle(aisle1, aisle2) // horizontally
		+ std::min(DistanceCellToBottom(cell1) + DistanceCellToBottom(cell2), DistanceTopToCell(cell1) + DistanceTopToCell(cell2));
}

int c_WarehouseLayout::DistanceDepotToPos(c_Position pos) const
{
	int aisle = pos.Aisle();
	int cell = pos.Cell();
	if (b_depot_at_bottom)
	{
		return DistanceAisleToAisle(aisle, i_depot_aisle) // horizontally
			+ i_dist_bottom_to_first_cell + v_cumulative_cell_to_cell_dist.back() - v_cumulative_cell_to_cell_dist[cell]; // vertically
	}
	// otherwise depot is at top
	return DistanceAisleToAisle(aisle, i_depot_aisle) // horizontally
		+ i_dist_top_to_last_cell + v_cumulative_cell_to_cell_dist[cell]; // vertically
}

// class c_OrderBatchingAndPickerRoutingProblem

// required default constructor
c_OrderBatchingAndPickerRoutingProblem::c_OrderBatchingAndPickerRoutingProblem() {}

void c_OrderBatchingAndPickerRoutingProblem::GenerateMuterOncanInstance(int seed, int num_orders, int capacity)
{
	//@TODO Add some kind of warehouse settings datastructure and file as MuterOncan doesnt have any.
	std::cout << "OBPRP with MuterOncan instance" << std::endl;
	i_multiplicator = 1;
	std::cout << "OBPRP multiplicator is " << i_multiplicator << std::endl;
	std::cout << "Warehouse dimensions are hardcoded according to Ratliff & Rosenthal" << std::endl;

	//for (int num_orders : { 20, 30, 40, 50, 60, 70, 80, 90, 100 })
	//{
	//	for (int capacity : { 24, 32, 48 })
	//	{
	//		for (int seed = 0; seed < 10; seed++)
	//		{

	const int depot_aisle = 0;
	const bool depot_at_bottom = true;
	const bool mirrored = true;
	const int num_aisles = 10;
	const int num_cells = 10;
	const int dist_top2last_cell = 0;
	const int dist_bottom2first_cell = 0;
	const int dist_top_or_bottom_to_depot = 0;
	std::vector<int> aisle2aisle_dist(num_aisles - 1, 24);
	std::vector<int> cell2cell_dist(num_cells - 1, 10);

	std::default_random_engine generator(seed);
	std::uniform_int_distribution<int> distribution_num_lines(2, 10);
	std::uniform_int_distribution<int> distribution_aisle(0, num_aisles - 1);
	std::uniform_int_distribution<int> distribution_cell(0, num_cells - 1);

	std::map<std::pair<int, int>, int> positions;
	for (int order = 0; order < num_orders; order++)
	{
		// random aisle and cell
		int num_orderlines = distribution_num_lines(generator);
		std::vector<c_OrderLine> ol_vec;
		for (int line = 0; line < num_orderlines; line++)
		{
			int article = -1;
			int aisle = distribution_aisle(generator);
			int cell = distribution_cell(generator);
			std::pair<int, int> pos(aisle, cell);
			if (positions.find(pos) == positions.end())
			{
				article = (int)positions.size();
				positions[pos] = article;
			}
			else
				article = positions[pos];
			ol_vec.push_back(c_OrderLine(article, 1));
		}
		v_orders.push_back(c_Order(ol_vec));
	}
	std::vector<c_SKU> SKUs;
	for (auto pos : positions)
	{
		int aisle = pos.first.first;
		int cell = pos.first.second;
		int article = pos.second;
		bool left = article % 2; // alternate between left and right :-)
		int quantity = 1;
		SKUs.push_back(c_SKU(article, aisle, cell, quantity, left));
	}
	o_warehouse = c_WarehouseLayout(aisle2aisle_dist, cell2cell_dist, dist_top2last_cell, dist_bottom2first_cell, depot_at_bottom, depot_aisle, dist_top_or_bottom_to_depot);
	o_SKUs = c_SKU_Collection(SKUs, true);
	i_capacity = capacity;
}

c_OrderBatchingAndPickerRoutingProblem::c_OrderBatchingAndPickerRoutingProblem(std::string filename, int format, std::string settingsFilename)
{
	if (filename == "")
	{
		// create example on the fly
		std::vector<int> aisle2aisle_dist{ 2,2,2,2,2 }; // 6 aisles, distance 2
		std::vector<int> cell2cell_dist(15, 1);  // 16 cells 0..15, distance 1
		int dist_top2last_cell = 0;
		int dist_bottom2first_cell = 0;
		bool depot_at_bottom = false;
		int depot_aisle = 1;
		o_warehouse = c_WarehouseLayout(aisle2aisle_dist, cell2cell_dist, dist_top2last_cell, dist_bottom2first_cell, depot_at_bottom, depot_aisle);

		std::vector<c_SKU> items;
		int article_id = 0;
		// aisle 0
		items.push_back(c_SKU(article_id++, 0, 4, 1, true));
		items.push_back(c_SKU(article_id++, 0, 7, 1, true));
		items.push_back(c_SKU(article_id++, 0, 12, 1, true));
		// aisle 1
		items.push_back(c_SKU(article_id++, 1, 4, 1, true));
		items.push_back(c_SKU(article_id++, 1, 10, 1, true));
		// aisle 2
		items.push_back(c_SKU(article_id++, 2, 3, 1, true));
		items.push_back(c_SKU(article_id++, 2, 6, 1, true));
		items.push_back(c_SKU(article_id++, 2, 12, 1, true));
		// aisle 3
		items.push_back(c_SKU(article_id++, 3, 15, 1, true));
		// aisle 4
		items.push_back(c_SKU(article_id++, 4, 8, 1, true));
		// aisle 5
		items.push_back(c_SKU(article_id++, 5, 3, 1, true));
		items.push_back(c_SKU(article_id++, 5, 6, 1, true));
		items.push_back(c_SKU(article_id++, 5, 13, 1, true));
		bool with_options = true;
		o_SKUs = c_SKU_Collection(items, with_options);

		const int seed = 1;
		const int max_num_orderlines = 3;
		const int max_quantity = 3;
		const int num_orders = 40;
		std::default_random_engine generator(seed);
		std::uniform_int_distribution<int> distribution_orderlines(1, max_num_orderlines);
		std::uniform_int_distribution<int> distribution_articles(0, article_id - 1);
		std::uniform_int_distribution<int> distribution_quantity(1, 3);
		for (int i = 0; i < num_orders; i++)
		{
			int num_orderlines = distribution_orderlines(generator);
			std::vector<c_OrderLine> vec;
			for (int j = 0; j < num_orderlines; j++)
				vec.push_back(c_OrderLine(distribution_articles(generator), distribution_quantity(generator)));
			v_orders.push_back(c_Order(vec));
		}
		// TODO
		int max_weight = *std::max_element(v_weight_of_order.begin(), v_weight_of_order.end());
		i_capacity = 3 * max_weight;
	}
	else if (format == MUTER_ONCAN)
	{
		// generate out of file name
		std::istringstream tokenStream(filename);
		std::string token;
		std::getline(tokenStream, token, '_');
		int num_orders = stoi(token);
		std::getline(tokenStream, token, '_');
		int capacity = stoi(token);
		std::getline(tokenStream, token, '_');
		int seed = stoi(token);
		std::cout << "Instance named <" << filename << "> has " << num_orders << " orders, a capacity of " << capacity << ", and random seed " << seed << "\n";
		GenerateMuterOncanInstance(seed, num_orders, capacity);
	}
	else if (format == HENN)
	{
		std::cout << "OBPRP with Henn instance" << std::endl;
		i_multiplicator = 2;
		int depot_aisle = 0;
		bool depot_at_bottom = false;
		bool mirrored = true;
		std::cout << "OBPRP multiplicator is " << i_multiplicator << std::endl;

		if (mirrored)
			depot_at_bottom = !depot_at_bottom;

		InstanceHenn in;
		in.setupInstance(filename, settingsFilename, depot_aisle, depot_at_bottom, i_multiplicator);

		// warehouse dimensions
		float aisleWidth = in.getAisleWidth()*i_multiplicator;
		float cellWidth = in.getCellWidth()*i_multiplicator;
		float cellLength = in.getCellLength()*i_multiplicator;
		int dist_top_or_bottom_to_depot = in.get_dis_ais_to_depot()*i_multiplicator;
		i_capacity = in.getCapacity();

		// Warehouse structure
		// Warehouse counts starting with 0
		int numAisles = in.getNumAisles() - 1;
		int numCells = in.getNumCells() - 1;

		std::vector<int> aisle2aisle_dist(numAisles, aisleWidth + 2 * cellWidth); // 6 aisles, distance 5 from center of aisle to next center of aisle along cross-aisle
		std::vector<int> cell2cell_dist(numCells, cellLength);  // 45 cells 0..44, distance 1. If left or right is stored in item, as Ratliff ignores it anyways.
		int dist_top2last_cell = cellLength;//center of cross-aisle is at 0.5
		int dist_bottom2first_cell = cellLength;//center of cross-aisle is at 0.5

		o_warehouse = c_WarehouseLayout(aisle2aisle_dist, cell2cell_dist, dist_top2last_cell, dist_bottom2first_cell, depot_at_bottom, depot_aisle, dist_top_or_bottom_to_depot);
		o_SKUs = c_SKU_Collection(placeHennInWarehouse(in, numCells, mirrored), false);
	}
	else
	{
		std::cerr << "not implemented yet";
		throw;
	}
	if (format == HENN)
	{
		std::cout << "Warning: v_orders in c_OrderBatchingAndPickerRoutingProblem is empty for HENN format" << std::endl;
	}
	//@TODO Fix instance type agnostic weight of order calculation!
	// compute the weight of all orders
	/*v_weight_of_article.resize(getNumSKUs());
	for (int i = 0; i < (int)v_weight_of_article.size(); i++)
		v_weight_of_article[i] = 1; //@TODO read weight from file if supportet from HENN file format!
	v_weight_of_order.resize(v_orders.size());
	for (int i = 0; i < v_orders.size(); i++)
	{
		int weight = 0;
		for (int line = 0; line < Order(i).NumberOfOrderLines(); line++)
		{
			auto ol = Order(i).Orderline(line);
			weight += ol.Quantity() * v_weight_of_article[ol.Article()];
		}
		v_weight_of_order[i] = weight;
	}*/

}

const c_WarehouseLayout & c_OrderBatchingAndPickerRoutingProblem::Warehouse() const
{
	return o_warehouse;
}

const c_SKU_Collection& c_OrderBatchingAndPickerRoutingProblem::SKUs() const
{
	return o_SKUs;
}

const c_Order & c_OrderBatchingAndPickerRoutingProblem::Order(int i) const
{
	return v_orders[i];
}

/*
* Return number of nodes consisting of all items and depot.
*/
int c_OrderBatchingAndPickerRoutingProblem::getNumNodes()
{
	return (int)o_SKUs.SKUs().size() + 1;
}

/*
* Return number of SKUs in Collection.
*/
int c_OrderBatchingAndPickerRoutingProblem::getNumSKUs()
{
	return (int)o_SKUs.SKUs().size();
}

void c_OrderBatchingAndPickerRoutingProblem::setAllOrders(std::vector<std::vector<int>> all_orders)
{
	this->vv_all_orders = all_orders;
}

void c_OrderBatchingAndPickerRoutingProblem::pushBackWeightOfOrder(int weight)
{
	v_weight_of_order.push_back(weight);
}

int c_OrderBatchingAndPickerRoutingProblem::NumOrders() const
{
	return (int)v_orders.size();
}

int c_OrderBatchingAndPickerRoutingProblem::NumAisles() const
{
	return o_warehouse.NumberOfAisles();
}

int c_OrderBatchingAndPickerRoutingProblem::NumCellsPerAisle() const
{
	return o_warehouse.NumberOfCellsPerAisle();
}

int c_OrderBatchingAndPickerRoutingProblem::NumArticles() const
{
	return o_SKUs.LargestArticleID() + 1;
}

int c_OrderBatchingAndPickerRoutingProblem::PickerCapacity()
{
	return i_capacity;
}

/*
* Returns weight of article i.
*/
int c_OrderBatchingAndPickerRoutingProblem::WeightOfArticle(int i)
{
	return v_weight_of_article[i];
}

int c_OrderBatchingAndPickerRoutingProblem::WeightOfOrder(int i)
{
	return v_weight_of_order[i];
}

// class c_Order

int c_Order::NumberOfOrderLines() const
{
	return (int)v_orderlines.size();
}

// class c_OrderLine

const c_OrderLine & c_Order::Orderline(int i) const
{
	return v_orderlines[i];
}

// Import all Henn Instances orders into the warehouse.
// Even shelves are leftside/true, uneven are right/false.
// Imports instance mirrored along y-axis to account for warehouse coordinate system having y=0 at top.
std::vector<c_SKU> c_OrderBatchingAndPickerRoutingProblem::placeHennInWarehouse(InstanceHenn in, int i_max_cell_index, bool mirrored)
{
	std::vector<c_SKU> items;
	int articleId = 0;
	int weight = 0;
	for (int j = 0; j < in.getNumOrders(); j++)
	{
		std::vector<std::array<int, 2>> order = in.getSingleOrderFromBatch(j);
		int numArticles = in.getNumArticlesInOrder(j);
		for (int i = 0; i < numArticles; i++)
		{
			int x = order[i][0];
			bool isLeft = true;

			if (x % 2)
			{
				x = x - 1;
				isLeft = false;
			}
			int y = order[i][1];
			if (mirrored)
				y = i_max_cell_index - order[i][1];//y-axis mirror
			items.push_back(c_SKU(articleId, (x / 2), y, 1, isLeft));
			weight++;
		}
		v_weight_of_order.push_back(weight);
		//pushBackWeightOfOrder(weight);
		weight = 0;
		articleId++;
	}
	//store relation between articleIDs and respective orders in warehouse
	setAllOrders(in.getAllOrders());
	return items;
}

/*

// Import all Muter&Öncan Instances orders into the warehouse.
// Even shelves are leftside/true, uneven are right/false.
// MuterOncan instances appear to start coordinates with 1, hence,
// coordinates are changed by 1 to start positioning instances from the upper, left corner.
std::vector<c_SKU> c_OrderBatchingAndPickerRoutingProblem::placeMuterOncanInWarehouse(InstanceMuterOncan in, int i_max_cell_index, bool mirrored)
{
	std::vector<c_SKU> items;
	int articleId = 0;
	for (int j = 0; j < in.getNumOrders(); j++)
	{
		std::vector<std::array<int, 2>> order = in.getSingleOrder(j);
		int num_orderlines = in.getNumArticlesInOrder(j);
		for (int i = 0; i < num_orderlines; i++)
		{
			int x = order[i][0] - 1;
			bool isLeft = true;
			if (x % 2)
			{
				x = x - 1;
				isLeft = false;
			}
			int y = order[i][1] - 1;
			if (mirrored)
				y = i_max_cell_index - order[i][1] + 1;//y-axis mirror
			items.push_back(c_SKU(articleId, (x / 2), y, 1, isLeft));
		}
		articleId++;
	}
	return items;
}

*/