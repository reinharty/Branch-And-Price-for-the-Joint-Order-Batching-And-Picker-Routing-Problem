#ifndef WAREHOUSE_H
#define WAREHOUSE_H

#include <tuple>
#include <vector>
#include <algorithm>

#include "InstanceHenn.cpp"
#include "InstanceMuterOncan.cpp"

struct c_Position : public std::pair<int, int> {
	c_Position(int aisle, int cell) : pair<int, int>(aisle, cell) {}
	int Aisle() const { return first; }
	int Cell() const { return second; }
};


struct c_SKU : public std::tuple<int, int, int, int, bool> {
	c_SKU(int article, int aisle, int cell, int quantity, bool left) : tuple<int, int, int, int, bool>(article, aisle, cell, quantity, left) {}

	int Article() const { return std::get<0>(*this); }
	int Aisle() const { return std::get<1>(*this); }
	int Cell() const { return std::get<2>(*this); }
	int Quantity() const { return std::get<3>(*this); }
	bool OnLeftHandSide() const { return std::get<4>(*this); }
	c_Position Position() const { return c_Position(Aisle(), Cell()); }
};


struct c_OrderLine : public std::tuple<int, int> {
	c_OrderLine(int article, int quantity) : std::tuple<int, int>(article, quantity) {}

	int Article() const { return std::get<0>(*this); }
	int Quantity() const { return std::get<1>(*this); }
};


class c_Order {
	std::vector<c_OrderLine> v_orderlines;
public:
	c_Order(const std::vector<c_OrderLine>& orderlines) : v_orderlines(orderlines) {}

	int NumberOfOrderLines() const;
	const c_OrderLine& Orderline(int i) const;
};


class c_SKU_Collection {
	// the vector that defines everything
	std::vector<c_SKU> v_all_SKUs;
	int i_largest_article_id;
	// help values and vectors
	int i_first_aisle_with_sku;
	int i_last_aisle_with_sku;
	bool b_scattered_storage;
	bool b_selection;
	std::vector<int> v_all_articles;
	std::vector<std::vector<c_SKU> > v_SKU_by_aisle;
	std::vector<std::vector<c_SKU> > v_SKU_of_article;//Is this articles per order?
	std::vector<std::vector<int> > v_non_empty_cells_in_aisle;
	std::vector<std::vector<std::vector<int> > > v_articles_in_aisle_at_pos;
	std::vector<std::vector<std::vector<int> > > v_articles_in_aisle_between_top_and_pos;
	std::vector<std::vector<std::vector<int> > > v_articles_in_aisle_between_pos_and_bottom;
	std::vector<std::vector<int> > v_articles_in_aisle_sorted_by_pos;
	std::vector<int> v_first_aisle_with_article;
	std::vector<int> v_last_aisle_with_article;
public:
	c_SKU_Collection(const std::vector<c_SKU>& all_SKUs, bool selection = false);
	c_SKU_Collection() {};

	const std::vector<c_SKU>& SKUs() const;;
	// articles
	const std::vector<int>& AllArticles() const;
	int LargestArticleID() const;
	// aisles
	const std::vector<c_SKU>& SKUsInAisle(int j) const; // order by cell
	int FirstAisleWithSKU() const;
	int LastAisleWithSKU() const;
	bool IsAisleEmpty(int i) const;
	int FirstAisleWithArticle(int a) const;
	int LastAisleWithArticle(int a) const;
	int NumberOfNonEmptyCellsInAisle(int aisle) const;
	const std::vector<int>& NonEmptyCellsInAisle(int aisle) const;
	const std::vector<int>& ArticlesInAisle(int aisle) const;
	const std::vector<int>& ArticlesInAisleBetweenTopAndPos(int aisle, int pos) const;
	const std::vector<int>& ArticlesInAisleBetweenPosAndBottom(int aisle, int pos) const;
	const std::vector<int>& ArticlesInAisleSortedByPos(int aisle) const;
	std::vector<int> ArticlesInAisleBetweenTopAndCell(int aisle, int cell) const;
	std::vector<int> ArticlesInAisleBetweenCellAndBottom(int aisle, int cell) const;
	const bool PositionIsRedundant(int aisle, int pos) const;
	const bool PositionsAreRedundant(int aisle, int pos1, int pos2) const;
	// article
	const std::vector<c_SKU>& SKUsOfArticle(int a) const; // ordered by aisle
	bool ArticleHasUniquePosition(int a) const;
	int TotalQuantity(int a) const;
	bool ArticleIsScattered(int a) const;
	bool ScatteredStorage() const;
	bool WithOptions() const;
};

/*
This is the representation we use for the physical  layout of the warehouse

					TOP

  \		Aisles (numbered left to right)
	\	aisle 0, aisle 1, aisle 2
Cells \
(numbered
bottom
up)
----------------------------------------------------
0
1
2
:
:
last-1
last
----------------------------------------------------

					BOTTOM
*/

class c_WarehouseLayout {
	std::vector<int> v_aisle_to_aisle_dist;
	std::vector<int> v_cell_to_cell_dist;
	int i_dist_top_to_last_cell;
	int i_dist_bottom_to_first_cell;
	bool b_depot_at_bottom;
	int i_depot_aisle;
	int i_dist_top_or_bottom_to_depot;
	// helpers
	std::vector<int> v_cumulative_aisle_to_aisle_dist;
	std::vector<int> v_cumulative_cell_to_cell_dist;
public:
	c_WarehouseLayout(
		const std::vector<int> aisle_to_aisle_dist, // implicit size()+1 is number of aisles
		const std::vector<int> cell_to_cell_dist, // implicit size()+1 is number of cells
		int dist_top_to_first_cell,
		int dist_bottom_to_last_cell,
		bool depot_at_bottom,
		int depot_aisle,
		int i_dist_top_or_bottom_to_depot = 0 //distance from depot to cross aisle
	);
	c_WarehouseLayout() {}

	int NumberOfAisles() const;
	int NumberOfCellsPerAisle() const;

	bool DepotIsAtTop() const;
	bool DepotIsAtBottom() const;
	int AisleOfDepot() const;

	int DistanceTopToBottom() const;
	int DistanceAisleToAisle(int i, int j) const;
	int DistanceCellToCell(int i, int j) const;
	int DistanceTopToCell(int i) const;
	int DistanceCellToBottom(int i) const;
	int DistancePosToPos(c_Position pos1, c_Position pos2) const;
	int DistanceDepotToPos(c_Position pos) const;
};


enum type { HENN, MUTER_ONCAN, UNKNOWN };

class c_OrderBatchingAndPickerRoutingProblem {
	c_WarehouseLayout o_warehouse;
	c_SKU_Collection o_SKUs; /* all articles stored somewhere in the warehouse */
	std::vector<c_Order> v_orders;
	std::vector<int> v_weight_of_article;
	std::vector<int> v_weight_of_order;
	std::vector<std::vector<int>> vv_all_orders;//stores orders with articleIDs representing instance structure
	int i_capacity;
	int i_multiplicator;
public:
	c_OrderBatchingAndPickerRoutingProblem();
	// generate in Muter & Öncan fashion
	void GenerateMuterOncanInstance(int seed, int num_orders, int capacity);
	c_OrderBatchingAndPickerRoutingProblem(std::string filename, int format, std::string settingsFilename = "");
	// getter
	const c_WarehouseLayout& Warehouse() const;
	const c_SKU_Collection& SKUs() const;
	const c_Order& Order(int i) const;
	int getNumNodes();
	int getNumSKUs();
	int getMultiplicator() { return i_multiplicator; }
	std::vector<std::vector<int>> getAllOrders() { return vv_all_orders; };
	std::vector<int> getWeightsOfOrders() { return v_weight_of_order; };
	// setter
	void setAllOrders(std::vector<std::vector<int>> all_orders);
	void pushBackWeightOfOrder(int weight);
	// --numbers
	int NumOrders() const;
	int NumAisles() const;
	int NumCellsPerAisle() const;
	int NumArticles() const;
	// --data
	int PickerCapacity();
	int WeightOfArticle(int i);
	int WeightOfOrder(int i);
	// read instance
	std::vector<c_SKU> placeHennInWarehouse(InstanceHenn in, int i_max_cell_index, bool mirrored);
	std::vector<c_SKU> placeMuterOncanInWarehouse(InstanceMuterOncan in, int i_max_cell_index, bool mirrored);
};

#endif