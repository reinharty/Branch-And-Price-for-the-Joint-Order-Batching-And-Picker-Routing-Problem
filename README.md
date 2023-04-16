# Master Thesis Code Excerpt

This repository contains code excerpts used in my master thesis.
The code is provided as-is.
You may use it for study and research.
It contains classes which represent instances of Henn2010 and MuterÖncan2015 and python scripts to generate instance files (and a sample of those) as described by Hwang2005.
A warehouse structure and a representation of the OBPRP are found in the warehouse files.
The problem solving code is found in the BaP files and will not run on its own. But it might give you an idea on how to solve similiar problems using IBM CPLEX.


## Abstract
The Joint Order Batching and Picker Routing Problem ("JOBPRP") is a cost minimizing problem which combines the Order Batching Problem ("OBP") and Picker Routing Problem ("PRP") into a single problem by solving them simultaneously. We show that instances for the JOBPRP can be solved through the state-of-the-art exact solution branch-and-price algorithm of Hintsch and Irnich (2020) which originally was intended for the Soft Clustered Vehicle Routing Problem ("SoftCluVRP"). We propose an alternative subproblem formulation based on Briant et al. (2020) and Ratliff and Rosenthal (1983) for the branch-and-price algorithm of Hintsch and Irnich (2020) and solve it through a cutting plane algorithm and demonstrate optimization strategies by solving subsets of resulting subproblem solution batches to faster find out which tours violate constraints and add cuts to the target plane. We find that we are able to solve instances with up to 80 orders and 1255 pick operations in less than one hour and outperform Hintsch and Irnich (2020) by two orders of magnitude.