#include "BaP_SoftCluVRP.h"
#include "DP.h"

using namespace std;



///////////////////////////////////////////////////////////////////////////////
// GenericCallback
///////////////////////////////////////////////////////////////////////////////

/*
* This is where the cut generator is called.
*/
void MyGenericCallback::invoke(const IloCplex::Callback::Context &context)
{
	//std::cout << "Here is the invoke" << endl;
	if (context.inCandidate())
		lazyCut2(context);
}

/*
* Returns a vector 'sequence' which starts with an incrementing sequence of numbers and is rearranged according to selected option
* Select method by setting mode to one of the following integers.
* 0 = default = does not change order
* 1 = Randomize using default_random_engine
* 2 = rearrange by ascending dual prices
* 3 = rearrange by descending dual prices
* 4 = rearrange by ascending dual price/order weight ratio
* 5 = rearrange by descending dual price/order weight ratio
* 6 = rearrange by ascending size of orders
*  7 = rearrange by descending size of orders
*/
std::vector<int> MyGenericCallback::rearrangeSequenceOfOrders(int mode, int size) {

	//sequence is used  to generate new random order
	std::vector<int> sequence;

	for (int i = 0; i < size; i++) {
		sequence.push_back(i);
	}

	switch (mode) {
		case 1:
			randomShuffle(1, sequence);
			break;
		case 2:
			sortByDualPrices(0, sequence);
			break;
		case 3:
			sortByDualPrices(1, sequence);
			break;
		case 4:
			sortByDualPricePerWeight(0, sequence);
			break;
		case 5:
			sortByDualPricePerWeight(1, sequence);
			break;
		case 6:
			sortBySizeOfOrder(0, sequence);
			break;
		case 7:
			sortBySizeOfOrder(1, sequence);
		default:
			break;
	}
	return sequence;
}

/*
* Sort sequence by size of orders.
* mode == 1 => sort descending
* else sort ascending
*/
void MyGenericCallback::sortBySizeOfOrder(int mode, std::vector<int>& sequence) {
	
	std::vector<int> sizes;
	std::vector<std::pair<int, int>> list;

	int x = o_obprp.SKUs().LargestArticleID();
	for (int i = 0; i <= o_obprp.SKUs().LargestArticleID(); i++) {
		sizes.push_back(o_obprp.SKUs().SKUsOfArticle(i).size());
	}
	//std::cout << o_obprp.getNumSKUs() << " " << sequence.size() << endl;

	//couple dual prices with indeces
	for (int i = 0; i < sequence.size(); i++) {
		list.emplace_back(sizes.at(i), i);
		//std::std::cout << list[i].first << " ";
	}

	//sort 
	if (mode != 1) {
		std::sort(list.begin(), list.end());
	}
	else {
		std::sort(list.rbegin(), list.rend());
	}

	//rearrange sequence
	for (int i = 0; i < sequence.size(); i++) {
		sequence.at(i) = list[i].second;
		//	std::std::cout << list[i].first << " ";
	}
	

}

/*
* Sort sequence by dual_prices/weight ratio.
* mode == 1 => sort descending
* else sort ascending
*/
void MyGenericCallback::sortByDualPricePerWeight(int mode, std::vector<int>& sequence) {

	std::vector<int> weight = o_obprp.getWeightsOfOrders();
	std::vector<std::pair<double, int>> list;

	//couple dual prices with indeces
	for (int i = 0; i < sequence.size(); i++) {
		list.emplace_back((v_dual_prices.at(i)/weight.at(i)), i);
		//std::std::cout << list[i].first << " ";
	}
	//std::std::cout << std::endl;

	//sort 
	if (mode != 1) {
		std::sort(list.begin(), list.end());
		//std::std::cout << "sort ascending" << endl;
	}
	else {
		std::sort(list.rbegin(), list.rend());
		//std::std::cout << "sort descending" << endl;
	}

	//rearrange sequence
	for (int i = 0; i < sequence.size(); i++) {
		sequence.at(i) = list[i].second;
		//std::std::cout << list[i].first << " ";
	}
	//std::std::cout << std::endl;
}

/*
* Sort sequence by dual_prices.
* mode == 1 => sort descending
* else sort ascending
*/
void MyGenericCallback::sortByDualPrices(int mode, std::vector<int>& sequence) {

	std::vector<std::pair<double, int>> list;
	
	//couple dual prices with indeces
	//std::cout << std::endl;
	for (int i = 0; i < sequence.size(); i++) {
		list.emplace_back(v_dual_prices.at(i), i);
		//std::cout << list[i].first << " ";
	}
	//std::cout << std::endl;

	//sort 
	if (mode != 1) {
		std::sort(list.begin(), list.end());
	}
	else {
		std::sort(list.rbegin(), list.rend());
	}

	//rearrange sequence
	for (int i = 0; i < sequence.size(); i++) {
		sequence.at(i) = list[i].second;
		//std::cout << list[i].first << " ";
	}
	//std::cout << std::endl;
}

/*
* Shuffles given vector.
* 0 = Use default_random_engine (usually Marsenne Twister) with its default seed
* 1 = Use ranlux24_base with seed = 0
* 2 = Use ranlux24_base with changing seed based on time
*/
void MyGenericCallback::randomShuffle(int mode, std::vector<int>& sequence) {

	switch (mode) {
	case 1:
		//for (int i = 0; i < 1000; i++) {} //might be needed to make some time pass between calls
		std::shuffle(std::begin(sequence), std::end(sequence), std::ranlux48_base(0));
		break;
	case 2:
		//for (int i = 0; i < 1000; i++) {} //might be needed to make some time pass between calls
		std::shuffle(std::begin(sequence), std::end(sequence), std::ranlux48_base(time(0)));
		break;
	default:
		std::shuffle(std::begin(sequence), std::end(sequence), std::default_random_engine());
		break;
	}
}

/*
This is the original lazy cut function.
*/
inline void MyGenericCallback::lazyCut(const IloCplex::Callback::Context &context) const
{
	//std::cout << "Here is the lazy cut CB" << endl;
	
	int dk = 0;
	int k = 0; //number of orders in ilo_ex

	std::vector<int> solution; //only needed for DP-function, never returned
	std::vector<c_SKU> items;
	c_SKU_Collection collection = o_obprp.SKUs();

	//get variables
	IloNumArray curr_ex(context.getEnv());

	context.getCandidatePoint(ilo_ex, curr_ex);

	double curr_d = context.getCandidateValue(ilo_d);

	//new cut
	IloExpr my_cut(context.getEnv());

	//calculate dk and k
	int counter = 0;
	bool ilo_exIsEmpty = true;

	vector<int> item_of_index;
	for (int i = 0; i < curr_ex.getSize(); i++) {

		if (curr_ex[i] >= (1.0-0.0001)) { //1-epsilon to care for float rounding errors.
			//std::cout << i <<" ";
			ilo_exIsEmpty = false;
			k++;//increment k for every order in ilo_ex
			my_cut += ilo_ex[i];//add ilo_ex entry which is 1 to cut
			for (int j = 0; j < collection.SKUs().size(); j++) {

				if (collection.SKUs().at(j).Article() == i) {
					c_SKU sku = collection.SKUs().at(j);
					items.push_back(c_SKU(counter, sku.Aisle(), sku.Cell(), sku.Quantity(), sku.OnLeftHandSide()));
					counter++;
					item_of_index.push_back(j);
				}
			}
		}
	}

	if (!ilo_exIsEmpty){
		c_SKU_Collection skus(items);
		dk = DP(o_obprp.Warehouse(), skus, e_forward, false, solution);
		for (int i = 0; i < solution.size(); i++)
			solution[i] = item_of_index[solution[i]] + 1;
		solution.push_back(0);
	}

	if (curr_d <= dk - 0.0001) {
		my_cut = dk * (my_cut - k + 1);
		my_cut -= ilo_d;
		context.rejectCandidate(my_cut <= 0.0);
		std::cout << "cut: " << my_cut << "<= 0" <<  endl;
	}
	else {
		//std::cout << my_cut << " not rejected" << endl;
	}
}

/*
* Start with subset and end with full batch or lazy cut.
* This is LCpricerS.
*/
inline void MyGenericCallback::lazyCut2(const IloCplex::Callback::Context &context)
{
	//std::cout << "Here is the lazy cut CB" << endl;

	//get variables
	IloNumArray curr_ex(context.getEnv());
	vector<int> v_curr_ex_copy;

	context.getCandidatePoint(ilo_ex, curr_ex);

	std::vector<int> sequence = rearrangeSequenceOfOrders(3, curr_ex.getSize());

	double curr_d = context.getCandidateValue(ilo_d);

	c_SKU_Collection collection = o_obprp.SKUs();
	
	bool loop = true;
	int foundOrders = 0;// counter on how many orders are found for subset.

	int subsetSize = curr_ex.getSize() * 0.8;//upper limit size of subset to start with when building subset of orders in first iteration. Is incremented up to curr_ex.getSize() if nothing is rejected?
	//std::cout << endl << curr_ex.getSize() << " " << subsetSize << endl;
	if (subsetSize > curr_ex.getSize()) {
		subsetSize = curr_ex.getSize();
		std::cout << "WARNING: Upper limit for subsets is higher than number of orders." << endl;
		std::cout << "subsetSize variable was set to curr_ex.getSize() which is " << subsetSize << endl;
	}

	while ((subsetSize <= curr_ex.getSize()) && (loop == true))
	{

		//loop over leading 0 in batch
		while (true) {
			if (!(curr_ex[sequence[subsetSize - 1]] >= (1.0 - 0.0001)) && (subsetSize < curr_ex.getSize())) {
				subsetSize++;
			}
			else {
				break;
			}
		}

		int dk = 0;
		int k = 0; //number of orders in ilo_ex

		//calculate dk and k
		int counter = 0;
		bool ilo_exIsEmpty = true;

		vector<int> item_of_index;

		//new cut
		IloExpr my_cut(context.getEnv());

		std::vector<int> solution; //only needed for DP-function, never returned
		std::vector<c_SKU> items;
	
		foundOrders = 0;	

		for (int i = 0; i < subsetSize; i++) {

			if ((curr_ex[sequence[i]] >= (1.0 - 0.0001)) && (foundOrders <= subsetSize)) { //1-epsilon to care for float rounding errors. 
				//std::cout << i <<" ";
				ilo_exIsEmpty = false;
				foundOrders++;
				k++;//increment k for every order in ilo_ex
				my_cut += ilo_ex[sequence[i]];//add ilo_ex entry which is 1 to cut
				for (int j = 0; j < collection.SKUs().size(); j++) {

					if (collection.SKUs().at(j).Article() == sequence[i]) {
						c_SKU sku = collection.SKUs().at(j);
						items.push_back(c_SKU(counter, sku.Aisle(), sku.Cell(), sku.Quantity(), sku.OnLeftHandSide()));
						counter++;
						item_of_index.push_back(j);
					}
				}
			}
		}

		if (!ilo_exIsEmpty) {
			c_SKU_Collection skus(items);
			dk = DP(o_obprp.Warehouse(), skus, e_forward, false, solution);
			for (int i = 0; i < solution.size(); i++)
				solution[i] = item_of_index[solution[i]] + 1;
			solution.push_back(0);
		}

		if (curr_d <= dk - 0.0001) {
			my_cut = dk * (my_cut - k + 1);
			my_cut -= ilo_d;
			context.rejectCandidate(my_cut <= 0.0);
			//std::cout << "cut: " << my_cut << "<= 0" << endl;
			loop = false;
		}
		else {
			subsetSize++;	
		}
	}
}

/*
* This function is for testing reorganization of the curr_ex sequence without a loop as in lazyCut2.
* This is LCpricer when rearrangeSequenceOfOrders is used with 0 which makes it skip any sorting.
*/
inline void MyGenericCallback::lazyCut3(const IloCplex::Callback::Context &context)
{
	//std::cout << "Here is the lazy cut CB" << endl;

	//dk == cost_optimal_route is INT_MAX if ilo_ex is all 0 to avoid erroneous shorter distances.
	int dk = 0;
	int k = 0; //number of orders in ilo_ex

	std::vector<int> solution; //only needed for DP-function, never returned
	std::vector<c_SKU> items;
	c_SKU_Collection collection = o_obprp.SKUs();

	//get variables
	IloNumArray curr_ex(context.getEnv());

	context.getCandidatePoint(ilo_ex, curr_ex);

	std::vector<int> sequence = rearrangeSequenceOfOrders(0, curr_ex.getSize());

	double curr_d = context.getCandidateValue(ilo_d);

	//new cut
	IloExpr my_cut(context.getEnv());

	//calculate dk and k
	int counter = 0;
	bool ilo_exIsEmpty = true;

	vector<int> item_of_index;
	for (int i = 0; i < curr_ex.getSize(); i++) {

		if (curr_ex[sequence[i]] >= (1.0 - 0.0001)) { //1-epsilon to care for float rounding errors.
			//std::cout << i <<" ";
			ilo_exIsEmpty = false;
			k++;//increment k for every order in ilo_ex
			my_cut += ilo_ex[sequence[i]];//add ilo_ex entry which is 1 to cut
			for (int j = 0; j < collection.SKUs().size(); j++) {

				if (collection.SKUs().at(j).Article() == sequence[i]) {
					c_SKU sku = collection.SKUs().at(j);
					items.push_back(c_SKU(counter, sku.Aisle(), sku.Cell(), sku.Quantity(), sku.OnLeftHandSide()));
					counter++;
					item_of_index.push_back(j);
				}
			}
		}
	}

	if (!ilo_exIsEmpty) {
		c_SKU_Collection skus(items);
		dk = DP(o_obprp.Warehouse(), skus, e_forward, false, solution);
		for (int i = 0; i < solution.size(); i++)
			solution[i] = item_of_index[solution[i]] + 1;
		solution.push_back(0);
	}

	if (curr_d <= dk - 0.0001) {
		my_cut = dk * (my_cut - k + 1);
		my_cut -= ilo_d;
		context.rejectCandidate(my_cut <= 0.0);
		//std::cout << "cut: " << my_cut << "<= 0" <<  endl;
	}
	else {
		//std::cout << my_cut << " not rejected" << endl;
	}
}

/*
* LazyCut which starts with all orders being part of route and removes order after order in each iteration.
* This is LCpricerG.
*/
inline void MyGenericCallback::lazyCut4(const IloCplex::Callback::Context &context)
{
	//std::cout << "Here is the lazy cut CB" << endl;

	std::vector<int> solution; //only needed for DP-function, never returned
	std::vector<c_SKU> items;

	c_SKU_Collection collection = o_obprp.SKUs();

	//get variables
	IloNumArray curr_ex(context.getEnv());

	context.getCandidatePoint(ilo_ex, curr_ex);

	std::vector<int> v_ex;
	for (int i = 0; i < curr_ex.getSize(); i++) {
		if (curr_ex[i] >= (1.0 - 0.0001)) {
			v_ex.push_back(1);
		}
		else {
			v_ex.push_back(0);
		}
	}

	std::vector<int> sequence = rearrangeSequenceOfOrders(2, curr_ex.getSize());

	double curr_d = context.getCandidateValue(ilo_d);

	//prior iteration cut
	IloExpr my_cut_prior(context.getEnv());
	int dk_prior = 0;
	int k_prior = 0;

	int c = 0;
	while (c < curr_ex.getSize()) {

		/*
		Skip iterations where at c is a 0, since that 0 is leading.
		New cut will be calculated when the next 1 is hit as this 1 will be changed to a leading 0 afterwards.
		*/
		if (v_ex[sequence[c]] >= (1.0 - 0.001)) {

			int dk = 0;
			int k = 0; //number of orders in ilo_ex

			//calculate dk and k
			int counter = 0;
			bool ilo_exIsEmpty = true;

			vector<int> item_of_index;

			//new cut
			IloExpr my_cut(context.getEnv());

			std::vector<int> solution; //only needed for DP-function, never returned
			std::vector<c_SKU> items;

			for (int i = 0; i < curr_ex.getSize(); i++) {

				if (v_ex[sequence[i]] >= (1.0 - 0.001)) { //1-epsilon to care for float rounding errors.
					//std::cout << i <<" ";
					ilo_exIsEmpty = false;
					k++;//increment k for every order in ilo_ex
					my_cut += ilo_ex[sequence[i]];//add ilo_ex entry which is 1 to cut
					for (int j = 0; j < collection.SKUs().size(); j++) {

						if (collection.SKUs().at(j).Article() == sequence[i]) {
							c_SKU sku = collection.SKUs().at(j);
							items.push_back(c_SKU(counter, sku.Aisle(), sku.Cell(), sku.Quantity(), sku.OnLeftHandSide()));
							counter++;
							item_of_index.push_back(j);
						}
					}
				}
			}

			if (!ilo_exIsEmpty) {
				c_SKU_Collection skus(items);
				dk = DP(o_obprp.Warehouse(), skus, e_forward, false, solution);
				for (int i = 0; i < solution.size(); i++)
					solution[i] = item_of_index[solution[i]] + 1;
				solution.push_back(0);
			}

			if (curr_d <= dk - 0.0001) {
				my_cut = dk * (my_cut - k + 1);
				my_cut -= ilo_d;
				context.rejectCandidate(my_cut <= 0.0);
				v_ex[sequence[c]] = 0;
			}
			else {
				break;
			}

		}
		else {

		}
		c++;
	}
}


/*
* Version of lazyCut4 without any sorting but with direct check on curr_ex and no substituting vector.
*/
inline void MyGenericCallback::lazyCut5(const IloCplex::Callback::Context &context) {
	
	std::vector<int> solution; //only needed for DP-function, never returned
	std::vector<c_SKU> items;
	c_SKU_Collection collection = o_obprp.SKUs();

	//get variables
	IloNumArray curr_ex(context.getEnv());
	context.getCandidatePoint(ilo_ex, curr_ex);
	double curr_d = context.getCandidateValue(ilo_d);

	int c = 0;
	while (c < curr_ex.getSize()) {
	
		if (curr_ex[c] >= (1.0 - 0.001)) {//dont process routes with leading 0s

			int dk = 0;
			int k = 0;
			int counter = 0;
			bool ilo_exIsEmpty = true;
			vector<int> item_of_index;
			vector<c_SKU> items;
			IloExpr my_cut(context.getEnv());
			solution.clear();
			
			for (int i = 0; i < curr_ex.getSize(); i++) {
				
				if (curr_ex[i] >= (1.0 - 0.001)) {
					ilo_exIsEmpty = false;
					k++;
					my_cut += ilo_ex[i];

					for (int j = 0; j < collection.SKUs().size(); j++) {

						if (collection.SKUs().at(j).Article() == i) {
							c_SKU sku = collection.SKUs().at(j);
							items.push_back(c_SKU(counter, sku.Aisle(), sku.Cell(), sku.Quantity(), sku.OnLeftHandSide()));
							counter++;
							item_of_index.push_back(j);
						}
					}
				}
			}

			if (!ilo_exIsEmpty) {
				c_SKU_Collection skus(items);
				dk = DP(o_obprp.Warehouse(), skus, e_forward, false, solution);
				for (int i = 0; i < solution.size(); i++)
					solution[i] = item_of_index[i] + 1;
				solution.push_back(0);
			}

			if (curr_d <= dk - 0.001) {

				my_cut = dk * (my_cut - k + 1);
				my_cut -= ilo_d;
			
				context.rejectCandidate(my_cut <= 0.0);
				curr_ex[c] = 0;

			}
			else {
				break;
			}
		}
		c++;
	}
}


/////////////////////////////////////////////////////////////////////////////
//   c_PricingBender_Model
/////////////////////////////////////////////////////////////////////////////

c_PricingBender_Model::c_PricingBender_Model(IloEnv& env, c_OrderBatchingAndPickerRoutingProblem& obprp)
	: IloModel(env),
	//Calculate size of array from largest ID in order context!
	ilo_ex(env, obprp.NumArticles(), 0, 1, ILOINT), //order is either part of route or not. 
	ilo_d(env, 0.0, 100000.0, ILOFLOAT),
	ilo_help(env, 1, 1, ILOINT),//wie relevant fuer das model in der MA oder nur spezifisch fuer kompatibilitaet mit framework?
	ilo_objective(env),
	ilo_env(env),
	o_obprp(obprp),
	d_pickerCapacity(obprp.PickerCapacity())
{
	IloExpr expr(env); //corresponds to objective
	IloExpr expr5(env); //corresponds to line (5) in Briant and (5.3b) in thesis
	IloExpr expr_at_least_one_order(env);//additional constraint which is missing in original Briant model

	ilo_d.setName("d");

	for (int i = 0; i < obprp.NumArticles(); i++) {
		std::stringstream ss;
		ss << "x_" << i;
		ilo_ex[i].setName(ss.str().c_str());
		expr += ilo_ex[i];//*beta = 1.0 //ilo_beta
		expr5 += ilo_ex[i] * o_obprp.WeightOfOrder(i);//sum of weight of all items in order i
		expr_at_least_one_order += ilo_ex[i];//*beta = 1.0 //ilo_beta
	}

	expr = ilo_d + ilo_help - expr;
	ilo_objective = IloMinimize(env, expr);
	add(ilo_objective);
	std::cout << expr << endl;

	//capacity constraint
	add(IloConstraint(expr5 <= d_pickerCapacity));

	add(IloConstraint(expr_at_least_one_order >= 1));
}

IloConstraint c_PricingBender_Model::TogetherConstraint(int c1, int c2)
{
	IloExpr lhs(ilo_env);
	lhs += ilo_ex[c1];
	lhs -= ilo_ex[c2];
	return IloConstraint(lhs == 0.0);
}

IloConstraint c_PricingBender_Model::SeparateConstraint(int c1, int c2)
{
	IloExpr lhs(ilo_env);
	lhs += ilo_ex[c1];
	lhs += ilo_ex[c2];
	return IloConstraint(lhs <= 1.0);
}

void c_PricingBender_Model::ChangeProfitInObjectiveFunction(std::vector<double> dual_price)
{
	//std::cout << "ChangeProfitInObjectiveFunction" << endl;
	//change \beta_o in objective
	for (int i = 0; i < dual_price.size(); i++) {

		ilo_objective.setLinearCoef(ilo_ex[i], (-1*dual_price[i]));
		//std::cout << ilo_ex[i] << " " << (-1*dual_price[i]) << endl;
	}

	//ilo_objective.setLinearCoefs(ilo_ex, dual_price);
}

void c_PricingBender_Model::ChangeMuInObjectiveFunction(double mu_value)
{
	ilo_objective.setLinearCoef(ilo_help, mu_value);
}

//is never used?
void c_PricingBender_Model::GetSolution()
{
	std::cout << "GetSolution" << endl;
	//change this method
	//...
}

/////////////////////////////////////////////////////////////////////////////
// class c_Benders_Pricer
/////////////////////////////////////////////////////////////////////////////

c_Benders_Pricer::c_Benders_Pricer(c_Controller* controller)
	: c_PricingProblem(controller),
	ilo_env(),
	ilo_cplex(ilo_env),
	//ilo_model(ilo_env),
	i_num_clusters(((c_ControllerSoftCluVRP*)Controller())->NumClusters()),
	i_num_nodes(((c_ControllerSoftCluVRP*)Controller())->NumNodes()),
	//o_bender_model(ilo_env, ((c_ControllerSoftCluVRP*)Controller())->GetOBPRP()),
	o_bender_model(nullptr),
	o_obprp(((c_ControllerSoftCluVRP*)Controller())->GetOBPRP()),
	v_together(((c_ControllerSoftCluVRP*)controller)->Together()),
	v_separated(((c_ControllerSoftCluVRP*)controller)->Separated()),
	i_depot_cluster_id(((c_ControllerSoftCluVRP*)controller)->NumClusters()),
	v_dual_price(((c_ControllerSoftCluVRP*)controller)->NumClusters(), -1.0),
	iterationCounter(0)
{
	c_ControllerSoftCluVRP* ctlr = ((c_ControllerSoftCluVRP*)Controller());
	o_bender_model = new c_PricingBender_Model(ilo_env, ctlr->GetOBPRP());
	std::cout << "Using Benders MIP" << endl;
	// cplex solving parameters
	ilo_cplex.setOut(ilo_env.getNullStream());
	ilo_cplex.setWarning(ilo_env.getNullStream());
	ilo_cplex.setParam(IloCplex::Param::Threads, 1);
	ilo_cplex.setParam(IloCplex::TiLim, 3600);
}

void c_Benders_Pricer::Update(c_BranchAndBoundNode* node)
{
	c_ControllerSoftCluVRP* ctlr = (c_ControllerSoftCluVRP*)Controller();
	ChangeBranchingDecisions(ctlr->Together(), ctlr->Separated());
}


/*
* Creates new model with respective together and separated constraints.
*/
void c_Benders_Pricer::ChangeBranchingDecisions(const vector<vector<int>>& together, const vector<vector<int>>& separated)
{
	o_bender_model = new c_PricingBender_Model(ilo_env, o_obprp);

	for (int c = 0; c < o_bender_model->o_obprp.NumArticles(); c++) {
		for (int t = 0; t < (int)together[c].size(); t++)
		{
			if (c < together[c][t])
				o_bender_model->add(o_bender_model->TogetherConstraint(c, together[c][t]));
		}
		for (int s = 0; s < (int)separated[c].size(); s++)
		{
			if (c < separated[c][s])
				o_bender_model->add(o_bender_model->SeparateConstraint(c, separated[c][s]));
		}
	}
	
	MyGenericCallback mycallback(o_obprp, o_bender_model->e(), o_bender_model->d(), v_dual_price);
	CPXLONG contextMask = 0;
	contextMask |= IloCplex::Callback::Context::Id::Candidate;// for lazy cuts
	// If contextMask is not zero we add the callback.
	if (contextMask != 0)
	{ //contextMask is 32 == 0x20 == CANDIDATE as expected
		ilo_cplex.use(&mycallback, contextMask);
	}

}

void c_Benders_Pricer::Update(c_DualVariables* dual)
{
	c_ControllerSoftCluVRP* ctlr = (c_ControllerSoftCluVRP*)Controller();
	c_DualVariablesSoftCluVRP* my_dual = (c_DualVariablesSoftCluVRP*)dual;

	o_bender_model->ChangeMuInObjectiveFunction(-my_dual->mu());

	double rho_all = my_dual->rho();
	for (int c = 0; c < i_num_clusters; c++) {
		if (c != ctlr->ClusterIndex(ctlr->Depot()))
			v_dual_price[c] = my_dual->pi(c) + rho_all;
	}
	//Remove depot cluster entry as not part of model
	//std::vector<double> v_dual_price_without_depot = v_dual_price;
	//v_dual_price_without_depot.erase(v_dual_price_without_depot.begin() + i_depot_cluster_id);
	o_bender_model->ChangeProfitInObjectiveFunction(v_dual_price);
}

double c_Benders_Pricer::RHSofConvexityConstraint()
{
	c_ControllerSoftCluVRP* ctlr = (c_ControllerSoftCluVRP*)Controller();
	return (double)ctlr->NumVehicles();
}

std::vector<double> c_Benders_Pricer::getDualPrices()
{
	return v_dual_price;
}

/*
* This Go method should be used if Solve function is used to solve the MIP.
*/
int c_Benders_Pricer::GoSolve(c_DualVariables* dual, list<c_ColumnPricePair>& cols_and_prices, double& best_val) {
	c_ControllerSoftCluVRP* ctlr = (c_ControllerSoftCluVRP*)Controller();
	c_TimeInfo timer_mip_pricer;
	timer_mip_pricer.Start();
	vector<int> order;//stores which order is part of corresponding route
	double rdc_per_route;
	double rdc = 0.0;

	try
	{
		ilo_cplex.extract(*o_bender_model);

		// debug
		ilo_cplex.exportModel("output/model.lp");

		MyGenericCallback mycallback(o_obprp, o_bender_model->e(), o_bender_model->d(), v_dual_price);
		CPXLONG contextMask = 0;
		contextMask |= IloCplex::Callback::Context::Id::Candidate;// for lazy cuts

		std::cout << ilo_cplex.isMIP() << " " << ilo_cplex.isQC() << endl;
		// If contextMask is not zero we add the callback.
		if (contextMask != 0)
		{ //contextMask is 32 == 0x20 == CANDIDATE as expected
			ilo_cplex.use(&mycallback, contextMask);
		}

		ilo_cplex.solve();
		std::cout << ilo_cplex.getStatus() << endl;

		if (ilo_cplex.getStatus() == IloAlgorithm::Optimal) {
			rdc = ilo_cplex.getObjValue();
			std::cout << "getObjValue rdc = " << rdc << endl;
		}
		else {
			rdc = ilo_cplex.getBestObjValue();
		}
		//std::cout << "convexity: " << d_dual_price_convexity << endl;
		//std::cout << "rdc = " << rdc << endl;

		best_val = rdc;
		
	}
	catch (IloCplex::Exception e)
	{
		std::cout << "error status: " << e.getStatus() << endl;
		std::cout << "error text:   " << e.getMessage() << endl;
		ilo_cplex.exportModel("error.lp");
		throw;
	}


	//get route and reduced costs of route

	IloNumArray curr_ex(ilo_env);
	ilo_cplex.getValues(curr_ex, o_bender_model->e());
	
	for (int j = 0; j < curr_ex.getSize(); j++)
	{
		if (curr_ex[j] >= 1.0 - 0.001)
			order.push_back(1);
		else
			order.push_back(0);
	}
	rdc_per_route = best_val;

	//ilo_cplex.writeSolutions("output/BenderMIPSolutions.txt");

	//what are the costs of the route? 
	if ((int)order.size() > 0 && best_val <= 0.0 - 0.00001)
		{
			int cost = 0;
			
			//routes contains integers of correlation to order, must be replaced with consecutive indices instead.
			//create new collection which only has SKUs which are part of this route
			//calculate cost of route using Ratliff.

			std::vector<int> route; //only needed for DP-function
			std::vector<c_SKU> items;
			c_SKU_Collection collection = o_obprp.SKUs();
			vector<int> r = order;
			int counter = 0;

			vector<int> item_of_index;
			for (int j = 0; j < r.size(); j++) {
				if (r[j] == 1) {
					for (int k = 0; k < o_obprp.getNumSKUs(); k++) {
						if (collection.SKUs().at(k).Article() == j) {
							c_SKU sku = collection.SKUs().at(k);// only for better readability of following line
							items.push_back(c_SKU(counter, sku.Aisle(), sku.Cell(), sku.Quantity(), sku.OnLeftHandSide()));
							counter++;
							item_of_index.push_back(k);
						}
					}
				}
			}

			c_SKU_Collection skus(items);
			cost = DP(o_obprp.Warehouse(), skus, e_forward, false, route);
			for (int x = 0; x < route.size(); x++)
				route[x] = item_of_index[route[x]] + 1;
			route.push_back(ctlr->OrigDepot());

			Normalize(route);
			route.push_back(ctlr->DestDepot());
			c_RouteColumnSoftCluVRP* new_col = new c_RouteColumnSoftCluVRP(ctlr, route, (double)cost);

			double rdc_test = new_col->ReducedCosts(*dual);

			if (fabs(rdc_test - rdc_per_route) > 0.1)
			{
				std::cout << "rdc_test = " << rdc_test << endl;
				std::cout << "rdc_per_route[" << 0 << "] = " << rdc_per_route << endl;
				std::cout << (fabs(rdc_test - rdc_per_route)) << " > 0.1 ? " << endl;
				std::cout << *dual << endl;
				std::cout << "branching together: " << endl;
				for (int i1 = 0; i1 < v_together.size(); i1++)
				{
					for (int i2 = 0; i2 < v_together[i1].size(); i2++)
					{
						std::cout << i1 << " " << v_together[i1][i2] << endl;
					}
				}
				std::cout << "branching separate: " << endl;
				for (int i1 = 0; i1 < v_separated.size(); i1++)
				{
					for (int i2 = 0; i2 < v_separated[i1].size(); i2++)
					{
						std::cout << i1 << " " << v_separated[i1][i2] << endl;
					}
				}
				std::cout << "rdc = " << rdc << endl;

				std::cout << "Wrong RDC!" << endl;
				throw;
				}
			cols_and_prices.push_back(c_ColumnPricePair(new_col, rdc_per_route));
	}

	timer_mip_pricer.Stop();
	return (int)cols_and_prices.size();
}

/*
* This Go method should be used if populate function is used to solve the MIP.
*/
int c_Benders_Pricer::GoPopulate(c_DualVariables* dual, list<c_ColumnPricePair>& cols_and_prices, double& best_val)
{
	c_ControllerSoftCluVRP* ctlr = (c_ControllerSoftCluVRP*)Controller();
	// find the basic tour columns
	c_TimeInfo timer_mip_pricer;
	timer_mip_pricer.Start();
	vector<vector<int> > orders(0); //@TODO stehen hier die einzelnen routen als folge von SKUs drin oder die zugehoerigen routen wie in ilo_ex?
	vector<double> rdc_per_route(0);
	//solve model
	double rdc = 0.0;
	int numsol = 1; //This increases in the populate part, if populate is used.

	try
	{
		ilo_cplex.extract(*o_bender_model);

		// debug
		ilo_cplex.exportModel("output/model.lp");

		MyGenericCallback mycallback(o_obprp, o_bender_model->e(), o_bender_model->d(), v_dual_price);
		CPXLONG contextMask = 0;
		contextMask |= IloCplex::Callback::Context::Id::Candidate;// for lazy cuts

		// If contextMask is not zero we add the callback.
		if (contextMask != 0)
		{ //contextMask is 32 == 0x20 == CANDIDATE as expected
			ilo_cplex.use(&mycallback, contextMask);
		}

		/* Set the solution pool relative gap parameter to obtain solutions
			of objective value within 10% of the optimal */
			//ilo_cplex.setParam(IloCplex::Param::MIP::Pool::RelGap, 0.1);
			//ilo_cplex.setParam(IloCplex::Param::MIP::Pool::Intensity, 4);
		ilo_cplex.setParam(IloCplex::Param::MIP::Limits::Populate, 20);

		ilo_cplex.populate();

		//std::cout << ilo_cplex.getStatus() << endl;

		int numsol = ilo_cplex.getSolnPoolNsolns();
		std::cout << "The solution pool contains " << numsol << " solutions." << endl;

		/* Some solutions are deleted from the pool because of the solution
		pool relative gap parameter */
		/*int numsolreplaced = ilo_cplex.getSolnPoolNreplaced();
		std::cout << numsolreplaced << " solutions were removed due to the solution pool relative gap parameter." << endl;
		std::cout << "In total, " << numsol + numsolreplaced << " solutions were generated." << endl;*/

		/* Get the average objective value of solutions in the solution pool */
		//std::cout << "The average objective value of the solutions is " << ilo_cplex.getSolnPoolMeanObjValue() << "." << endl << endl;

		for (int i = 0; i < numsol; i++) {
			/* Write out objective value */
			rdc = ilo_cplex.getObjValue(i);
			//std::cout << "Solution " << i << " with objective rdc " << rdc << endl;

			rdc_per_route.push_back(rdc);
		}
		best_val = *std::min_element(std::begin(rdc_per_route), std::end(rdc_per_route));
	}
	catch (IloCplex::Exception e)
	{
		std::cout << "error status: " << e.getStatus() << endl;
		std::cout << "error text:   " << e.getMessage() << endl;
		ilo_cplex.exportModel("error.lp");
		throw;
	}


	//get routes and reduced costs of routes
	IloNumArray curr_ex(ilo_env);
	//std::cout << ilo_cplex.getSolnPoolNsolns() << endl;

	numsol = ilo_cplex.getSolnPoolNsolns();

	for (int i = 0; i < numsol; i++) {
		ilo_cplex.getValues(curr_ex, o_bender_model->e(), i);
		//std::cout << i << ". curr_ex: " << curr_ex << endl;
		vector<int> order;
		for (int j = 0; j < curr_ex.getSize(); j++)
		{
			if (curr_ex[j] >= 1.0 - 0.0001)
				order.push_back(1);
			else
				order.push_back(0);
		}
		orders.push_back(order);
	}

	//ilo_cplex.writeSolutions("output/BenderMIPSolutions.txt");

	for (int m = 0; m < rdc_per_route.size(); m++) {

		if ((int)orders[m].size() > 0 && best_val <= 0.0 - 0.00001)
		{
			int cost = 0;

			//routes contains integers of correlation to order, must be replaced with consecutive indices instead.
			//create new collection which only has SKUs which are part of this route
			//calculate cost of route using Ratliff.

			std::vector<int> route; //only needed for DP-function
			std::vector<c_SKU> items;
			c_SKU_Collection collection = o_obprp.SKUs();
			vector<int> r = orders[m];
			int counter = 0;

			vector<int> item_of_index;
			for (int j = 0; j < r.size(); j++) {
				if (r[j] == 1) {
					for (int k = 0; k < o_obprp.getNumSKUs(); k++) {
						if (collection.SKUs().at(k).Article() == j) {
							c_SKU sku = collection.SKUs().at(k);// only for better readability of following line
							items.push_back(c_SKU(counter, sku.Aisle(), sku.Cell(), sku.Quantity(), sku.OnLeftHandSide()));
							counter++;
							item_of_index.push_back(k);
						}
					}
				}
			}

			c_SKU_Collection skus(items);
			cost = DP(o_obprp.Warehouse(), skus, e_forward, false, route);
			for (int x = 0; x < route.size(); x++)
				route[x] = item_of_index[route[x]] + 1;
			route.push_back(ctlr->OrigDepot());

			Normalize(route);
			route.push_back(ctlr->DestDepot());
			c_RouteColumnSoftCluVRP* new_col = new c_RouteColumnSoftCluVRP(ctlr, route, (double)cost);

			double rdc_test = new_col->ReducedCosts(*dual);

			if (fabs(rdc_test - rdc_per_route[m]) > 0.1)
			{
				std::cout << "rdc_test = " << rdc_test << endl;
				std::cout << "rdc_per_route[" << m << "] = " << rdc_per_route[m] << endl;
				std::cout << (fabs(rdc_test - rdc_per_route[m])) << " > 0.1 ? " << endl;
				std::cout << "cost = " << cost << endl;
				std::cout << *dual << endl;
				std::cout << "branching together: " << endl;
				for (int i1 = 0; i1 < v_together.size(); i1++)
				{
					for (int i2 = 0; i2 < v_together[i1].size(); i2++)
					{
						std::cout << i1 << " " << v_together[i1][i2] << endl;
					}
				}
				std::cout << "branching separate: " << endl;
				for (int i1 = 0; i1 < v_separated.size(); i1++)
				{
					for (int i2 = 0; i2 < v_separated[i1].size(); i2++)
					{
						std::cout << i1 << " " << v_separated[i1][i2] << endl;
					}
				}
				cout << "rdc = " << rdc << endl;

				std::cout << "Wrong RDC!" << endl;
				throw;
			}
			cols_and_prices.push_back(c_ColumnPricePair(new_col, rdc_per_route[m]));
		}
	}

	timer_mip_pricer.Stop();

	return (int)cols_and_prices.size();
}

/*
* This function is called to solve the model.
* If b_multiple_solutions is true, populate is used.
*/
int c_Benders_Pricer::Go(c_DualVariables* dual, list<c_ColumnPricePair>& cols_and_prices, double& best_val)
{
	//solve model
	bool b_multiple_solutions = true;

	if (b_multiple_solutions == false) {
		GoSolve(dual, cols_and_prices, best_val);
	}
	else {
		GoPopulate(dual, cols_and_prices, best_val);
	}

	return (int)cols_and_prices.size();
}

/*
* This is an experimental multithreaded Go function using populate.
* Is slower than single threaded function, probably due to mutexes locking to many variables as the remaining code is not optimized towards threading.
*/
int c_Benders_Pricer::GoPopulateThreads(c_DualVariables* dual, list<c_ColumnPricePair>& cols_and_prices, double& best_val)
{
	c_ControllerSoftCluVRP* ctlr = (c_ControllerSoftCluVRP*)Controller();
	// find the basic tour columns
	c_TimeInfo timer_mip_pricer;
	timer_mip_pricer.Start();
	vector<vector<int> > orders(0); //@TODO stehen hier die einzelnen routen als folge von SKUs drin oder die zugehoerigen routen wie in ilo_ex?
	vector<double> rdc_per_route(0);
	//solve model
	double rdc = 0.0;
	int numsol = 1; //This increases in the populate part, if populate is used.

	try
	{
		ilo_cplex.extract(*o_bender_model);

		// debug
		ilo_cplex.exportModel("output/model.lp");

		MyGenericCallback mycallback(o_obprp, o_bender_model->e(), o_bender_model->d(), v_dual_price);
		CPXLONG contextMask = 0;
		contextMask |= IloCplex::Callback::Context::Id::Candidate;// for lazy cuts

		// If contextMask is not zero we add the callback.
		if (contextMask != 0)
		{ //contextMask is 32 == 0x20 == CANDIDATE as expected
			ilo_cplex.use(&mycallback, contextMask);
		}

		ilo_cplex.populate();

		std::cout << ilo_cplex.getStatus() << endl;

		int numsol = ilo_cplex.getSolnPoolNsolns();
		std::cout << "The solution pool contains " << numsol << " solutions." << endl;

		for (int i = 0; i < numsol; i++) {
			/* Write out objective value */
			rdc = ilo_cplex.getObjValue(i);
			std::cout << "Solution " << i << " with objective rdc " << rdc << endl;
			//best_val = rdc;
			rdc_per_route.push_back(rdc);
		}
		best_val = *std::min_element(std::begin(rdc_per_route), std::end(rdc_per_route));
	}
	catch (IloCplex::Exception e)
	{
		std::cout << "error status: " << e.getStatus() << endl;
		std::cout << "error text:   " << e.getMessage() << endl;
		ilo_cplex.exportModel("error.lp");
		throw;
	}
	IloNumArray curr_ex(ilo_env);

	numsol = ilo_cplex.getSolnPoolNsolns();

	for (int i = 0; i < numsol; i++) {
		ilo_cplex.getValues(curr_ex, o_bender_model->e(), i);
		//std::cout << i << ". curr_ex: " << curr_ex << endl;
		vector<int> order;
		for (int j = 0; j < curr_ex.getSize(); j++)
		{
			if (curr_ex[j] >= 1.0 - 0.0001)
				order.push_back(1);
			else
				order.push_back(0);
		}
		orders.push_back(order);
	}

	int thread_max=numsol;
	vector<thread> v_thread_pool;
	v_thread_pool.reserve(thread_max);

	for (int thread_num = 0; thread_num < thread_max; thread_num++) {
		v_thread_pool.push_back(thread([&] { postprocessing(ctlr, dual, cols_and_prices, best_val, rdc_per_route, orders, thread_num, thread_max); }));
	}
	for (int thread_num = 0; thread_num < thread_max; thread_num++) {
		v_thread_pool.at(thread_num).join();
	}
	//thread t1([&] { postprocessing(ctlr, dual, cols_and_prices, best_val, rdc_per_route, orders, 0, 1); });
    //t1.join();

	//postprocessing(ctlr, dual, cols_and_prices, best_val, rdc_per_route, orders, 0, 1);

	timer_mip_pricer.Stop();

	return (int)cols_and_prices.size();
}

/*
* Experimental postprocessing of CPLEX results. 
* Slow, don't use this.
*/
void c_Benders_Pricer::postprocessing(c_ControllerSoftCluVRP * ctlr, c_DualVariables* dual, list<c_ColumnPricePair>& cols_and_prices, double& best_val, vector<double> rdc_per_route, vector<vector<int>> orders, int thread_num, int thread_max) {
	//std::cout << "thread " << thread_num << endl;
	for (int m = thread_num; m < rdc_per_route.size(); m=m+thread_max) {
		//std::cout << "thread looped " << m << endl;
		if ((int)orders[m].size() > 0 && best_val <= 0.0 - 0.00001)
		{
			int cost = 0;

			std::vector<int> route; //only needed for DP-function
			std::vector<c_SKU> items;
			c_SKU_Collection collection = o_obprp.SKUs();
			vector<int> r = orders[m];
			int counter = 0;

			vector<int> item_of_index;
			for (int j = 0; j < r.size(); j++) {
				if (r[j] == 1) {
					for (int k = 0; k < o_obprp.getNumSKUs(); k++) {
						if (collection.SKUs().at(k).Article() == j) {
							c_SKU sku = collection.SKUs().at(k);// only for better readability of following line
							items.push_back(c_SKU(counter, sku.Aisle(), sku.Cell(), sku.Quantity(), sku.OnLeftHandSide()));
							counter++;
							item_of_index.push_back(k);
						}
					}
				}
			}

			c_SKU_Collection skus(items);
			cost = DP(o_obprp.Warehouse(), skus, e_forward, false, route);
			for (int x = 0; x < route.size(); x++)
				route[x] = item_of_index[route[x]] + 1;

			route.push_back(ctlr->OrigDepot());
			Normalize(route); //@error there is no depot in route?!?
			route.push_back(ctlr->DestDepot());
			c_RouteColumnSoftCluVRP* new_col = new c_RouteColumnSoftCluVRP(ctlr, route, (double)cost);

			double rdc_test = new_col->ReducedCosts(*dual);

			cols_and_prices.push_back(c_ColumnPricePair(new_col, rdc_per_route[m]));
		}
	}
}


/*
* Export RDC info by exporting errorWrongRDC.lp to output folder.
* Prints sizes and content of together and separated lists from controller to terminal.
*/
void c_Benders_Pricer::exportDebugInfoRDC() {
	c_ControllerSoftCluVRP* ctlr = (c_ControllerSoftCluVRP*)Controller();
	
	ilo_cplex.exportModel("output/errorWrongRDC.lp");
	
	int sizeT = ctlr->Together().size();
	int sizeS = ctlr->Separated().size();
	std::cout << "sizeT = " << sizeT << endl;
	std::cout << "sizeS = " << sizeS << endl;
}

void c_Benders_Pricer::Normalize(vector<int>& tour)
{
	c_ControllerSoftCluVRP* ctlr = (c_ControllerSoftCluVRP*)Controller();
	int start_pos = -1;
	int n = (int)tour.size();
	for (int pos = 0; pos < n; pos++)
		if (tour[pos] == ctlr->OrigDepot())
			start_pos = pos;
	if (start_pos < 0)
		throw std::logic_error("There is no depot in the route.");
	if (start_pos == 0)
		return; // nothing to do
	vector<int> new_tour(tour.size());
	for (int pos = 0; pos < tour.size(); pos++)
		new_tour[pos] = tour[(pos + start_pos) % n];
	tour = new_tour;
}