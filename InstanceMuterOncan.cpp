#include <vector>
#include <array>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>

// Compatibel to MuterOncan instances found in instances_timo folder.
// This class represents an instance file.
// It does not contain a warehouse datastructure.
// The instance holds number of orders.
// Each order starts with a line with one number representing the amount of order lines, 
// followed by lines of two numbers, the first representing shelve index, the second the cell index.
// The instance file does not hold any information on demand, capacity or number of vehicles.
struct InstanceMuterOncan
{
private:
	int i_no_orders;
	int i_no_order_lines;
	//int i_multiplicator;

	// store the highest indices in instance to check if warehouse is large enough for the instance to fit in
	int i_highest_shelve_index;
	int i_highest_cell_index;

	//vector of number of articles for each order
	std::vector<int> v_no_articles_per_order;

	//2-dim vector of orders
	//each coordinate is an array with two entries
	std::vector<std::vector<std::array<int, 2>>> vva_orders;

public:

	void setHighestShelveIndex(int i) {
		i_highest_shelve_index = i;
	}

	int getHighestShelveIndex() {
		return i_highest_shelve_index;
	}

	int getHighestCellIndex() {
		return i_highest_cell_index;
	}

	int getNumAisles() {
		int x = i_highest_shelve_index - 1;
		if (x % 2) {
			x = x - 1;
		}
		return (x / 2);
	}

	int getNumOrders() {
		return i_no_orders;
	}

	int getNumOrderLines() {
		return i_no_order_lines;
	}

	std::vector<std::array<int, 2>> getSingleOrder(int i) {
		return vva_orders.at(i);
	}

	int getNumArticlesInOrder(int i) {
		return (int)getSingleOrder(i).size();
	}

	int readInstance(std::string filename) {
		int i_no_orderlines = 0;
		int i_counter = 0;
		int i_x, i_y;
		bool b_in_order = false;
		bool b_first_line = true;

		i_highest_cell_index = 0;
		i_highest_shelve_index = 0;
		i_no_order_lines = 0;

		std::vector<std::array<int, 2>> order;
		// std::array<int, 2> coord;

		std::ifstream file(filename.c_str(), std::ifstream::in);

		if (file.fail()) {
			std::cout << "Failed reading from instance file." << std::endl;
			return 1;
		}

		std::string line;

		while (getline(file, line)) {

			std::stringstream ss(line);

			ss << line;

			if (b_first_line == true) {
				ss >> i_no_orders;
				b_first_line = false;
			}
			else if (b_first_line == false && b_in_order == false) {
				ss >> i_no_orderlines;
				b_in_order = true;
			}
			else if (b_first_line == false && b_in_order == true) {
				ss >> i_x >> i_y;

				order.push_back({ i_x, i_y });

				if (i_x > i_highest_shelve_index) {
					i_highest_shelve_index = i_x;
				}
				if (i_y > i_highest_cell_index) {
					i_highest_cell_index = i_y;
				}

				i_counter++;
				if (i_counter >= i_no_orderlines) {
					vva_orders.push_back(order);
					order.clear();
					i_no_order_lines += i_no_orderlines;
					i_counter = 0;
					b_in_order = false;
				}
			}
		}
		return 0;
	}
};