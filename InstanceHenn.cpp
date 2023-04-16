#include <vector>
#include <array>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>

//Compatible to Henn2010 instances
struct InstanceHenn
{
private:
	int i_no_aisles;//There are 2 shelves per aisle, therefore the Aisles mentiones in instance.txt are shelves from now on.
	int i_no_shelves;// = no_aisles * 2; //each aisle has shelves on left and right of it
	int i_no_cells; //no of cells per shelve
	int i_no_orderparts; //total number of individual order elements including cells appearing more than once
	int i_capacity;
	int i_multiplicator;
	float f_cell_len;//defined as base length unit LU //horizontal way
	float f_cell_width; // vertical way // is 5*LU in Henn2010
	float f_aisle_width;
	//float dis_ctc; //center to center distance between two aisles
	float f_dis_ais_wa; //distance between depot and cross aisle center


	// depot position
	int i_depot_shelve; //at which shelve, on x axis
	bool b_depot_at_upper; //next to cells at hight 0 or at height cell_number, on y axis

	//orderbatch<order<[shelve, cell]>>
	std::vector<std::vector<std::array<int, 2>>> vv_order_shelve_cell;


	//num of articles per order
	std::vector<int> v_num_articles_per_order;


	// Needed for analogeous behaviour to gvrp instance.
	// Same sizes as vv_orderbatch but stores corresponding index found inside va_coords; 
	std::vector<std::vector<int>> vv_all_orders;
	std::vector<std::array<int, 2>> va_coords; //at 0 is depot
	std::vector<int> vi_weight_node;//weight or demand of objects at cell. Corresponds to va_coords.
	std::vector<int> vi_demand_order; //sum of demand of each order

	/*
	d 1 2 3 4 5
	1
	2
	3
	4
	5
	*/
	std::vector<std::vector<float>> vv_distance_matrix;

public:

	int printNumberOfOrders() {
		std::cout << "Number of orders: " << vv_order_shelve_cell.size() << std::endl;
		return 0;
	}

	// Get x or y of coordinate i.
	int get_x_Coord(int i) { return va_coords[i][0]; }
	int get_y_Coord(int i) { return va_coords[i][1]; }

	// Get amount of aisles
	int getNumAisles() { return i_no_aisles; }

	// Get amount of cells per shelve
	int getNumCells() { return i_no_cells; }

	// Get capacity of picker.
	int getCapacity() { return i_capacity; }

	// Get real distance from cell of distance matrix.
	float get_real_distance(int i, int j) { return vv_distance_matrix[i][j]; }

	// Get distance between depot and cross aisle center
	float get_dis_ais_to_depot() { return f_dis_ais_wa; }

	// Get width of parallel aisle
	float getAisleWidth() { return f_aisle_width; }

	// Get width of cell
	float getCellWidth() { return f_cell_width; }

	// Get length of cell
	float getCellLength() { return f_cell_len; }

	// Returns distance multiplied by a half of LU to account for distance between middle of horizontal aisle and depot.
	// Won't work properly if LU is rational number.
	int get_natural_distance(int i, int j) { return (int)(get_real_distance(i, j) / (f_cell_len / i_multiplicator)); }

	// Opposite operation to get_natural_distance-function.
	int convert_to_real_distance(int i) { return (int)(i*(f_cell_len / 2)); }

	std::vector<int> get_cluster_members(int i) { return vv_all_orders[i]; }
	// Returns number of orders.
	int getNumOrders() { return (int)vv_order_shelve_cell.size(); }

	// Returns number of orderparts.
	int getNumNodes() { return i_no_orderparts; }

	// Get demand of one item
	int getDemandAt(int i) { return vi_weight_node.at(i); }

	// Get demand of 
	std::vector<int>getDemandVector() { return vi_weight_node; }

	// Returns demand of a single order / cluster. 
	// TODO: only works if each demand is 1!!! Must be changed if demands are different!
	int getDemandForOrder(int i) {
		return (int)vv_order_shelve_cell[i].size();
	}

	//Returns number of articles inside an order.
	int getNumArticlesInOrder(int order) { return v_num_articles_per_order[order]; }

	//Returns a single order from orderbatch.
	std::vector<std::array<int, 2>> getSingleOrderFromBatch(int order) { return vv_order_shelve_cell[order]; }

	// Returns ID representation of instance with 1st dimension representing orders and 2nd dimension storing related article IDs.
	std::vector<std::vector<int>> getAllOrders() { return vv_all_orders; }

	// Print whole orderbatch to console.
	int printOrderBatch() {

		for (int i = 0; i < v_num_articles_per_order.size(); i++) {
			std::cout << "Order " << i << " number of articles " << v_num_articles_per_order[i] << std::endl;

			for (int j = 0; j < v_num_articles_per_order[i]; j++) {
				std::cout << j << "\tShelve " << vv_order_shelve_cell[i][j][0] << "\tLocation " << vv_order_shelve_cell[i][j][1] << std::endl;
			}
		}

		return 0;
	}

	// Set position of depot. 
	// Depot is not set by default.
	// Must be set before reading instance.txt to ensure that depot is at coords[0].
	int setDepotPosition(int shelve, bool is_depot_at_max_y) {
		i_depot_shelve = shelve;
		b_depot_at_upper = is_depot_at_max_y;
		va_coords.push_back({ shelve, (is_depot_at_max_y*(i_no_cells - 1)) });
		vi_weight_node.push_back(0);
		return 0;
	}

	int setMultiplicator(int m) {
		i_multiplicator = m;
		return 0;
	}

	// Read instance from file.
	// Adds all coords of orderparts to coords array.
	// setDepotPosition first!
	int readInstance(std::string filename) {

		i_no_orderparts = 0;
		std::ifstream file(filename.c_str(), std::ifstream::in);

		if (file.fail()) {
			std::cout << "Failed reading from instance file." << std::endl;
			return 1;
		}

		std::string line;
		std::string sw;
		int counter_order = 0;
		int no_a = 0;
		int x = 0;
		int y = 0;
		int sum_demand = 0;
		std::string waste;
		std::vector<std::array<int, 2>> order;
		std::vector<int> coord_index;
		std::stringstream ss;
		++i_no_orderparts;//depot is first coordinate!!!

		while (getline(file, line)) {

			std::stringstream ss(line);

			ss << line;

			if (line.find("Order") != std::string::npos) {

				if (counter_order > 0) {
					vv_order_shelve_cell.push_back(order);
					vv_all_orders.push_back(coord_index);
					vi_demand_order.push_back(sum_demand);
					sum_demand = 0;
					order.clear();
					coord_index.clear();
				}

				ss >> waste >> waste >> waste >> waste >> waste >> no_a;
				v_num_articles_per_order.push_back(no_a);
				++counter_order;
			}
			else if (line.find("Aisle") != std::string::npos) {
				ss >> waste >> waste >> x >> waste >> y;
				order.push_back({ x, y });
				coord_index.push_back(i_no_orderparts);
				++i_no_orderparts;
				va_coords.push_back({ x, y });
				vi_weight_node.push_back(1);
				++sum_demand;
			}

		}

		vi_demand_order.push_back(sum_demand);
		sum_demand = 0;
		vv_order_shelve_cell.push_back(order);
		vv_all_orders.push_back(coord_index);
		order.clear();
		coord_index.clear();
		std::cout << "Total order parts + depot: " << i_no_orderparts << std::endl;
		return 0;
	}

	// Reads settings from file.
	int readSettings(std::string filename) {

		std::ifstream file(filename.c_str(), std::ifstream::in);

		if (file.fail()) {
			std::cout << "Failed reading from settings file." << std::endl;
			return 1;
		}

		std::string line;
		std::string sw;

		int i = 0;
		while (i < 7) {

			if (std::getline(file, line)) {
				std::stringstream ss(line);
				//std::cout << ss.str() << std::endl;
				ss >> sw;
				//std::cout << line << std::endl;
				//std::cout << "sw: " << sw << std::endl;

				if (sw.compare("no_aisles_:") == 0) {
					ss >> i_no_aisles;
					i_no_shelves = i_no_aisles * 2;
					std::cout << "no_aisles_:" << i_no_aisles << std::endl;
					std::cout << "no_shelves:" << i_no_shelves << std::endl;
					++i;
				}
				if (sw.compare("no_cells__:") == 0) {
					ss >> i_no_cells;
					std::cout << "no_cells__:" << i_no_cells << std::endl;
					std::cout << "Total no of cells:" << (i_no_cells*i_no_shelves) << std::endl;
					++i;
				}
				if (sw.compare("cell_lengt:") == 0) {
					ss >> f_cell_len;
					std::cout << "cell_lengt:" << f_cell_len << std::endl;
					++i;
				}
				if (sw.compare("cell_width:") == 0) {
					ss >> f_cell_width;
					std::cout << "cell_width:" << f_cell_width << std::endl;
					++i;
				}
				if (sw.compare("aisle_widt:") == 0) {
					ss >> f_aisle_width;
					std::cout << "aisle_widt:" << f_aisle_width << std::endl;
					++i;
				}
				if (sw.compare("dis_ais_wa:") == 0) {
					ss >> f_dis_ais_wa;
					std::cout << "dis_ais_wa:" << f_dis_ais_wa << std::endl;
					++i;
				}
				if (sw.compare("m_no_a_p_b:") == 0) {
					ss >> i_capacity;
					std::cout << "m_no_a_p_b/capacity:" << i_capacity << std::endl;
					++i;
				}
			}
			else {
				std::cout << line << std::endl;
				std::cout << "Error while reading file. Could not find all settings." << std::endl;
				file.close();
				return 1;
			}
		}
		file.close();
		return 0;
	}

	//Compute distance in LU between two cells.
	//Even x and a are left of aisle, uneven are right of aisle.
	// x is aisle, y is cell
	float getDistanceBetween2Cells(int shelve_x, int location_y, int shelve_a, int location_b) {

		int y = location_y;
		int x = shelve_x;
		int b = location_b;
		int a = shelve_a;

		if (x >= (i_no_shelves) || a >= (i_no_shelves)) {
			std::cout << "Shelve number higher than existing number of shelves!" << std::endl;
			return -1;
		}
		if (y >= i_no_cells || b >= i_no_cells) {
			std::cout << "Cell number higher than existing number of cells!" << std::endl;
			return -1;
		}

		float w1, w2;

		//only even number represent a cell on a graph representation, ignoring left and right
		if (x % 2) {
			--x;
		}
		if (a % 2) {
			--a;
		}

		// in same aisle, compute distance on y axis only
		if (x == a) {
			w1 = (float)std::abs(y - b);
			return w1;
		}

		// calc number of aisles from number of shelves
		x = x * 0.5;
		a = a * 0.5;

		b = b * f_cell_len;
		y = y * f_cell_len;
		float offset = (i_no_cells - 1)*f_cell_len;

		//move down towards 0
		// movement out and in aisles, step in and out of cross aisle, step along aisles on cross aisle
		w1 = y + b + (f_cell_len * 2) + (2 * f_cell_len + 2 * f_cell_width)*std::abs(x - a);
		// move upwards towards max no cell
		w2 = (offset - y) + (offset - b) + (f_cell_len * 2) + (2 * f_cell_len + 2 * f_cell_width)*std::abs(x - a);

		return std::min(w1, w2);
	}

	//Compute distance in LU between two cells.
	//Even x and a are left of aisle, uneven are right of aisle.
	// x is aisle, y is cell
	float getDistanceFromCellToDepot(int shelve, int location) {

		if (shelve % 2) {
			--shelve;
		}
		if (i_depot_shelve % 2) {
			--i_depot_shelve;
		}

		float w = getDistanceBetween2Cells(shelve, location, i_depot_shelve, (int(b_depot_at_upper)*(i_no_cells - 1)));

		if (shelve == i_depot_shelve) {
			w = w + 1.5;
		}
		else {
			w = w - 0.5;
		}
		return w;

	}

	// Compute distance matrix
	int calcDistanceMatrix() {

		std::vector<std::vector<float>> parts;
		// std::array<int, 2> coord;
		std::vector<float> tmp(i_no_orderparts);
		parts.resize(i_no_orderparts, tmp); //all parts plus depot
		int c = 0;


		for (int i = 0; i < (i_no_orderparts); i++) {
			for (int j = 0 + i; j < (i_no_orderparts); j++) {

				if (i == j) {
					parts[i][j] = 0;
					parts[j][i] = 0;
				}
				else if (i == 0 || j == 0) {
					parts[i][j] = getDistanceFromCellToDepot(va_coords[j][0], va_coords[j][1]);
					parts[j][i] = parts[i][j];
				}
				else {
					parts[i][j] = getDistanceBetween2Cells(va_coords[i][0], va_coords[i][1], va_coords[j][0], va_coords[j][1]);
					parts[j][i] = parts[i][j];
				}

			}
		}

		vv_distance_matrix = parts;

		//printDistanceMatrix();

		return 0;
	}

	// Print distance matrix with legend
	int printDistanceMatrix() {

		std::cout << "\tdepot";
		for (int i = 0; i < vv_distance_matrix.size() - 1; i++) {
			std::cout << "\t" << i << ".";
		}
		std::cout << std::endl;


		for (int i = 0; i < vv_distance_matrix.size(); i++)
		{
			if (i == 0) {
				std::cout << "\ndepot";
			}
			else {
				std::cout << (i - 1) << ".";
			}
			for (int j = 0; j < vv_distance_matrix[i].size(); j++)
			{
				std::cout << "\t" << vv_distance_matrix[i][j];
			}
			std::cout << std::endl;
		}

		return 0;
	}

	int setupInstance(std::string instancePath, std::string settingsPath, int aisle_of_depot, bool is_depot_at_x0, int multiplicator) {
		setDepotPosition(aisle_of_depot, is_depot_at_x0);
		setMultiplicator(multiplicator);
		readSettings(settingsPath);
		readInstance(instancePath);
		//printNumberOfOrders();
		//std::cout << "ctc distance: " << getDistanceBetween2Cells(3, 2, 5, 4) << std::endl;
		//calcDistanceMatrix();
		std::cout << "Instance was set up." << std::endl;
		return 0;
	}

};
