// Copyright (c) 2008, Jacob Burnim (jburnim@cs.berkeley.edu)
//
// This file is part of CREST, which is distributed under the revised
// BSD license.  A copy of this license can be found in the file LICENSE.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See LICENSE
// for details.

#include <algorithm>
#include <assert.h>
#include <ctype.h>
#include <fstream>
#include <limits>
#include <set>
#include <sstream>
#include <stdio.h>
#include <string>
#include <vector>
#include <ext/hash_map>

using namespace std;
using __gnu_cxx::hash_map;

typedef pair<int,int> edge_t;
typedef vector< pair<int,size_t> > adj_list_t;
typedef adj_list_t::iterator nbhr_it;
typedef adj_list_t::const_iterator const_nbhr_it;
typedef vector<adj_list_t> graph_t;
typedef set<int>::const_iterator BranchIt;

namespace __gnu_cxx {
  template<> struct hash<string> {
    size_t operator()(const string& x) const {
      return hash<const char*>()(x.c_str());
    }
  };
}

void readBranches(set<int>* branches) {
  ifstream in("branches");

  int fid, numBranches;
  while (in >> fid >> numBranches) {
    for (int i = 0;  i < numBranches; i++) {
      int b1, b2;
      assert(in >> b1 >> b2);
      branches->insert(b1);
      branches->insert(b2);
    }
  }

  in.close();
}

void readCfg(graph_t* graph) {
  // First we have to read in the function -> CFG node map.
  hash_map<string,int> funcNodeMap;
  { ifstream in("cfg_func_map");
    string func;
    int sid;
    while (in >> func >> sid) {
      funcNodeMap[func] = sid;
    }
    in.close();
  }

  // No we can read in the CFG edges, substituting the correct CFG nodes
  // for function calls.
  ifstream in("cfg");

  string line;
  while (getline(in, line)) {
    istringstream line_in(line);
    int src;
    line_in >> src;
    line_in.get();

    if (graph->size() <= src)
      graph->resize(src+1);
    adj_list_t& nbhrs = (*graph)[src];

    while (line_in) {
      if (isdigit(line_in.peek())) {
	int dst;
	line_in >> dst;
	nbhrs.push_back(make_pair(dst, 0));
      } else {
	string func;
	line_in >> func;
	hash_map<string,int>::iterator it = funcNodeMap.find(func);
	if (it != funcNodeMap.end())
	  nbhrs.push_back(make_pair(it->second,0));
      }
      line_in.get();
    }
  }

  in.close();
}

void dijkstra_bounded_shortest_paths
(const graph_t& g, int src, vector<size_t>& dist_map, size_t max_dist) {

  // Initialize all distances.
  for (int i = 0; i < g.size(); i++) {
    dist_map[i] = numeric_limits<size_t>::max();
  }
  dist_map[src] = 0;

  // Initialize the queue.
  set< pair<size_t, int> > Q;
  Q.insert(make_pair(0, src));

  // Initialize the sets of seen and finished vertices.
  set<int> finished;

  while (!Q.empty()) {
    // Pop the head of the queue.
    int v = Q.begin()->second;
    size_t dist = Q.begin()->first;
    Q.erase(Q.begin());

    // If the current distance is greater than the max, we're done.
    if (dist > max_dist)
      break;

    // Otherwise, process the vertex.
    finished.insert(v);
    for (const_nbhr_it e = g[v].begin(); e != g[v].end(); ++e) {
      int u = e->first;
      if (finished.find(u) == finished.end()) {
        size_t d = dist_map[v] + e->second;
        if ((d < dist_map[u]) && (d <= max_dist)) {
          Q.erase(make_pair(dist_map[u], u));
          Q.insert(make_pair(d, u));
          dist_map[u] = d;
        }
      }
    }
  }
}


int main(void) {

  // Read in the set of branches.
  set<int> branches;
  readBranches(&branches);
  fprintf(stderr, "Read %d branches.\n", branches.size());

  // Read in the CFG.
  graph_t cfg;
  cfg.reserve(1000000);
  readCfg(&cfg);
  fprintf(stderr, "Read %d nodes.\n", cfg.size());

  // Set the length of every edge to 1 if the destination is a branch,
  // and zero otherwise.
  for (size_t i = 0; i < cfg.size(); i++) {
    for (nbhr_it j = cfg[i].begin(); j != cfg[i].end(); ++j) {
      if (branches.find(j->first) != branches.end()) {
	j->second = 1;
      } else {
	j->second = 0;
      }
    }
  }

  std::ofstream out("cfg_branches", std::ios::out | std::ios::binary);
  size_t len = branches.size();
  out.write((char*)&len, sizeof(len));

  // "Thin" the graph down to unit-length edges between branches by, for each
  // branch, running Dijstrak's Algorithm until all other branches distance
  // one away have been discovered.  We print out an adjacency list for the
  // thinned graph as we go.
  vector<size_t> dist(cfg.size());
  vector<int> nbhrs;
  int numEdges = 0;
  for (BranchIt i = branches.begin(); i != branches.end(); ++i) {
    // Compute all shortest-paths of length zero and one from vertex i.
    dijkstra_bounded_shortest_paths(cfg, *i, dist, 1);

    // Accumulate the branch nodes distance one from i.
    nbhrs.clear();
    for (BranchIt j = branches.begin(); j != branches.end(); ++j) {
      if (dist[*j] == 1) {
	nbhrs.push_back(*j);
      }
    }
    numEdges += nbhrs.size();

    // Write out the neighbors.
    len = nbhrs.size();
    int dest = *i;
    out.write((char*)&dest, sizeof(dest));
    out.write((char*)&len, sizeof(len));
    out.write((char*)&nbhrs.front(), len * sizeof(int));
  }

  out.close();
  fprintf(stderr, "Wrote %d branch edges.\n", numEdges);
  return 0;
}
