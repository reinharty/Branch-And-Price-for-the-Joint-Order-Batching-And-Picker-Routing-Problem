

#include <ilcplex/ilocplex.h>
#include <math.h>
#include <random>
#include <numeric>
#include <algorithm> // for sort()
#include <thread>

class MyGenericCallback : public IloCplex::Callback::Function
{
private:
	IloNumVarArray ilo_ex;		//1 if order is part of route, else 0
	IloNumVar ilo_d;
	c_OrderBatchingAndPickerRoutingProblem o_obprp;
	std::vector<double> v_dual_prices;

public:
	int counter;
	MyGenericCallback(const c_OrderBatchingAndPickerRoutingProblem& obprp, const IloNumVarArray& vars_x, const IloNumVar& var_d, const std::vector<double> dual_prices) :
		ilo_ex(vars_x),
		ilo_d(var_d),
		o_obprp(obprp),
		v_dual_prices(dual_prices)
	{
	}
	// This is the function that we have to implement and that CPLEX will call
	// during the solution process at the places that we asked for.
	void invoke(const IloCplex::Callback::Context &context);
	std::vector<int> rearrangeSequenceOfOrders(int randomization, int size);
	void sortBySizeOfOrder(int mode, std::vector<int>& sequence);
	void sortByDualPricePerWeight(int mode, std::vector<int>& sequence);
	void sortByDualPrices(int mode, std::vector<int>& sequence);
	void randomShuffle(int mode, std::vector<int>& sequence);
	inline void lazyCut(const IloCplex::Callback::Context &context) const;
	inline void lazyCut2(const IloCplex::Callback::Context & context);
	inline void lazyCut3(const IloCplex::Callback::Context & context);
	inline void lazyCut4(const IloCplex::Callback::Context & context);
	inline void lazyCut5(const IloCplex::Callback::Context & context);
};

class c_PricingBender_Model :public IloModel
{
public:
	//contains all orders, distances and warehouse info
	c_OrderBatchingAndPickerRoutingProblem o_obprp;
	const double d_pickerCapacity;
	//variables of the papers model
	IloNumVarArray ilo_ex;		//1 if order is part of route, else 0
	IloNumVar ilo_d;			//total distance	
	IloNumVar ilo_help;
	IloObjective ilo_objective; //objective function
	IloEnv ilo_env;				//store env

	c_PricingBender_Model(IloEnv& env, c_OrderBatchingAndPickerRoutingProblem& obprp);
	IloConstraint TogetherConstraint(int c1, int c2);
	IloConstraint SeparateConstraint(int c1, int c2);
	virtual ~c_PricingBender_Model() {}
	void ChangeProfitInObjectiveFunction(std::vector<double> dual_price);
	void ChangeMuInObjectiveFunction(double mu_value);
	void GetSolution();
	void AddConstraint89();
	void AddConstraint(int i);
	void ResetConstraints();
	IloNumVar& e_a(int a) { return ilo_ex[a]; }
	IloNumVarArray& e() { return ilo_ex; }
	IloNumVar& d() { return ilo_d; }
};

class c_Benders_Pricer : public c_PricingProblem
{
private:
	//cplex
	IloEnv ilo_env;
	IloCplex ilo_cplex;
	//IloModel ilo_model;
	//
	c_OrderBatchingAndPickerRoutingProblem o_obprp;
	int i_num_nodes;
	int i_num_clusters;
	int iterationCounter;
	std::vector<double> v_dual_price;
	int i_depot_cluster_id;
	const std::vector<vector<int> >& v_together;
	const std::vector<vector<int> >& v_separated;
public:
	c_PricingBender_Model* o_bender_model;
	c_Benders_Pricer(c_Controller* controller);
	void Normalize(vector<int>& tour);
	// these methods can be modified
	virtual void Update(c_BranchAndBoundNode* node);
	void ChangeBranchingDecisions(const vector<vector<int>>& together, const vector<vector<int>>& separated);
	virtual void Update(c_DualVariables* dual);
	virtual int  Go(c_DualVariables* dual, list<c_ColumnPricePair>& cols_and_prices, double& best_val);
	int GoPopulateThreads(c_DualVariables * dual, list<c_ColumnPricePair>& cols_and_prices, double & best_val);
	void postprocessing(c_ControllerSoftCluVRP * ctlr, c_DualVariables * dual, list<c_ColumnPricePair>& cols_and_prices, double & best_val, vector<double> rdc_per_route, vector<vector<int>> orders, int threadIndex, int threadNum);
	void exportDebugInfoRDC();
	virtual double RHSofConvexityConstraint();
	std::vector<double> getDualPrices();
	int GoSolve(c_DualVariables * dual, list<c_ColumnPricePair>& cols_and_prices, double & best_val);
	int GoPopulate(c_DualVariables * dual, list<c_ColumnPricePair>& cols_and_prices, double & best_val);
};

class c_PricingSolverHierarchySoftCluVRP : public c_PricingProblemHierarchy {
public:
	c_PricingSolverHierarchySoftCluVRP(c_Controller* controller, int max_num_failures, int min_num_columns);

	virtual void Update(c_DualVariables* dual);
	virtual void Update(c_BranchAndBoundNode* node);
};
