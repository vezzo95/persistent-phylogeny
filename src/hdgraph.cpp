#include "hdgraph.hpp"


//=============================================================================
// Boost functions (overloading)

// Hasse Diagram

HDVertex
add_vertex(
    const std::list<std::string>& species,
    const std::list<std::string>& characters,
    HDGraph& hasse) {
  HDVertex v = boost::add_vertex(hasse);
  hasse[v].species = species;
  hasse[v].characters = characters;
  
  return v;
}

std::pair<HDEdge, bool>
add_edge(
    const HDVertex u,
    const HDVertex v,
    const std::list<SignedCharacter>& signedcharacters,
    HDGraph& hasse) {
  HDEdge e;
  bool exists;
  std::tie(e, exists) = boost::add_edge(u, v, hasse);
  hasse[e].signedcharacters = signedcharacters;
  
  return std::make_pair(e, exists);
}


//=============================================================================
// General functions

// Hasse Diagram

std::ostream& operator<<(std::ostream& os, const HDGraph& hasse) {
  HDVertexIter v, v_end;
  std::tie(v, v_end) = vertices(hasse);
  for (; v != v_end; ++v) {
    os << "[ ";
    
    std::list<std::string>::const_iterator i = hasse[*v].species.begin();
    for (; i != hasse[*v].species.end(); ++i) {
      os << *i << " ";
    }
    
    os << "( ";
    
    i = hasse[*v].characters.begin();
    for (; i != hasse[*v].characters.end(); ++i) {
      os << *i << " ";
    }
    
    os << ") ]:";
    
    HDOutEdgeIter e, e_end;
    std::tie(e, e_end) = out_edges(*v, hasse);
    for (; e != e_end; ++e) {
      HDVertex vt = target(*e, hasse);
      
      os << " -";
      
      SignedCharacterIter j = hasse[*e].signedcharacters.begin();
      for (; j != hasse[*e].signedcharacters.end(); ++j) {
        os << *j;
        
        if (std::next(j) != hasse[*e].signedcharacters.end())
          os << ",";
      }
      
      os << "-> [ ";
      
      i = hasse[vt].species.begin();
      for (; i != hasse[vt].species.end(); ++i) {
        os << *i << " ";
      }
      
      os << "( ";
      
      i = hasse[vt].characters.begin();
      for (; i != hasse[vt].characters.end(); ++i) {
        os << *i << " ";
      }
      
      os << ") ];";
    }
    
    if (std::next(v) != v_end)
      os << std::endl;
  }
  
  return os;
}


//=============================================================================
// Algorithm functions

bool
is_included(const std::list<std::string>& a, const std::list<std::string>& b) {
  std::list<std::string>::const_iterator i = a.begin();
  for (; i != a.end(); ++i) {
    std::list<std::string>::const_iterator in;
    in = std::find(b.begin(), b.end(), *i);
    
    if (in == b.end())
      // exit the function at the first string of a not present in b
      return false;
  }
  
  return true;
}

void hasse_diagram(HDGraph& hasse, const RBGraph& g) {
  std::vector<std::list<RBVertex>> sets(num_species(g));
  std::map<RBVertex, std::list<RBVertex>> v_map;
  
  // how sets is going to be structured:
  // sets[index] => < S, List of characters adjacent to S >
  
  // how v_map is going to be structured:
  // v_map[S] => < List of characters adjacent to S >
  
  // sets is used to sort the lists by number of elements, this is why we store
  // the list of adjacent characters to S. While we store S to be able to
  // access v_map[S] in costant time, instead of iterating on sets to find the
  // correct list
  
  // initialize sets and v_map for each species in the graph
  size_t index = 0;
  RBVertexIter v, v_end;
  std::tie(v, v_end) = vertices(g);
  for (; v != v_end; ++v) {
    if (!is_species(*v, g))
      continue;
    // for each species vertex
    
    #ifdef DEBUG
    std::cout << "C(" << g[*v].name << ") = { ";
    #endif
    
    // sets[index]'s first element is the species vertex
    sets[index].push_back(*v);
    
    // build v's set of adjacent characters
    RBOutEdgeIter e, e_end;
    std::tie(e, e_end) = out_edges(*v, g);
    for (; e != e_end; ++e) {
      // vt = one of the characters adjacent to *v
      RBVertex vt = target(*e, g);
      
      #ifdef DEBUG
      std::cout << g[vt].name << " ";
      #endif
      
      // sets[index]'s other elements are the characters adjacent to S
      sets[index].push_back(vt);
      // v_map[S]'s elements are the characters adjacent to S
      v_map[*v].push_back(vt);
    }
    
    #ifdef DEBUG
    std::cout << "}" << std::endl;
    #endif
    
    index++;
  }
  
  #ifdef DEBUG
  std::cout << std::endl;
  #endif
  
  // sort sets by size in ascending order
  std::sort(sets.begin(), sets.end(), ascending_size);
  
  for (size_t i = 0; i < sets.size(); ++i) {
    // for each set of characters
    // v = species of g
    RBVertex v = sets[i].front();
    
    // fill the list of characters names of v
    std::list<std::string> lcv;
    RBVertexIter cv = v_map[v].begin(), cv_end = v_map[v].end();
    for (; cv != cv_end; ++cv) {
      lcv.push_back(g[*cv].name);
    }
    
    if (i == 0) {
      // first iteration of the loop:
      // add v to the Hasse diagram, and being the first vertex of the graph
      // there's no need to do any work.
      #ifdef DEBUG
      std::cout << "Hasse.addV " << g[v].name << std::endl << std::endl;
      #endif
      
      add_vertex(g[v].name, lcv, hasse);
      
      continue;
    }
    
    #ifdef DEBUG
    std::cout << "C(" << g[v].name << ") = { ";
    
    std::list<std::string>::const_iterator kk = lcv.begin();
    for (; kk != lcv.end(); ++kk) {
      std::cout << *kk << " ";
    }
    
    std::cout << "}:" << std::endl;
    #endif
    
    // new_edges will contain the list of edges that may be added to the Hasse
    // diagram: HDVertex is the source, std::string is the edge label
    std::list<std::pair<HDVertex, std::string>> new_edges;
    
    // check if there is a vertex with the same characters as v or if v needs
    // to be added to the Hasse diagram
    HDVertexIter hdv, hdv_end;
    std::tie(hdv, hdv_end) = vertices(hasse);
    for (; hdv != hdv_end; ++hdv) {
      // for each vertex in hasse
      #ifdef DEBUG
      std::cout << "hdv: ";
      
      kk = hasse[*hdv].species.begin();
      for (; kk != hasse[*hdv].species.end(); ++kk) {
        std::cout << *kk << " ";
      }
      
      std::cout << "= { ";
      
      kk = hasse[*hdv].characters.begin();
      for (; kk != hasse[*hdv].characters.end(); ++kk) {
        std::cout << *kk << " ";
      }
      
      std::cout << "} -> ";
      #endif
      
      if (lcv == hasse[*hdv].characters) {
        // v and hdv have the same characters
        #ifdef DEBUG
        std::cout << "mod" << std::endl;
        #endif
        
        // add v to the list of species in hdv
        hasse[*hdv].species.push_back(g[v].name);
        
        break;
      }
      
      std::list<std::string> lhdv = hasse[*hdv].characters;
      
      // TODO: check if edge building is done right
      
      // initialize new_edges if lhdv is a subset of lcv, with the structure:
      // *hdv -*ci-> v
      if (is_included(lhdv, lcv)) {
        // hdv is included in v
        std::list<std::string>::const_iterator ci = lcv.begin(), ci_end = lcv.end();
        for (; ci != ci_end; ++ci) {
          // for each character in hdv
          std::list<std::string>::const_iterator in;
          in = std::find(lhdv.begin(), lhdv.end(), *ci);
          
          if (in == lhdv.end()) {
            // character is not present in lhdv
            new_edges.push_back(std::make_pair(*hdv, *ci));
          }
        }
      }
      
      // on the last iteration of the cycle, after having collected all the
      // pairs in new_edges, add vertex and edges to the Hasse diagram
      if (std::next(hdv) == hdv_end) {
        // last iteration on the characters in the list has been performed
        #ifdef DEBUG
        std::cout << "add" << std::endl;
        #endif
        
        // build a vertex for v and add it to the Hasse diagram
        HDVertex u = add_vertex(g[v].name, lcv, hasse);
        
        // build in_edges for the vertex and add them to the Hasse diagram
        std::list<std::pair<HDVertex, std::string>>::iterator ei, ei_end;
        ei = new_edges.begin(), ei_end = new_edges.end();
        for (; ei != ei_end; ++ei) {
          // for each new edge to add to the Hasse diagram
          HDEdge edge;
          std::tie(edge, std::ignore) = add_edge(ei->first, u, hasse);
          hasse[edge].signedcharacters.push_back({ ei->second, State::gain });
          
          #ifdef DEBUG
          std::cout << "Hasse.addE ";
          
          kk = hasse[ei->first].species.begin();
          for (; kk != hasse[ei->first].species.end(); ++kk) {
            std::cout << *kk << " ";
          }
          
          std::cout << "-";
          
          SignedCharacterIter jj = hasse[edge].signedcharacters.begin();
          for (; jj != hasse[edge].signedcharacters.end(); ++jj) {
            std::cout << *jj;
            
            if (std::next(jj) != hasse[edge].signedcharacters.end())
              std::cout << ",";
          }
          
          std::cout << "-> ";
          
          kk = hasse[u].species.begin();
          for (; kk != hasse[u].species.end(); ++kk) {
            std::cout << *kk << " ";
          }
          
          std::cout << std::endl;
          #endif
        }
        
        break;
      }
      #ifdef DEBUG
      else {
        std::cout << "ignore";
      }
      
      std::cout << std::endl;
      #endif
    }
    
    #ifdef DEBUG
    std::cout << std::endl;
    #endif
  }
  
  #ifdef DEBUG
  std::cout << "Before transitive reduction:" << std::endl
            << hasse << std::endl << std::endl;
  #endif
  
  // transitive reduction of the Hasse diagram
  HDVertexIter u, u_end;
  std::tie(u, u_end) = vertices(hasse);
  for (; u != u_end; ++u) {
    if (in_degree(*u, hasse) == 0 || out_degree(*u, hasse) == 0)
      continue;
    // for each internal vertex in hasse
    
    HDInEdgeIter ie, ie_end;
    std::tie(ie, ie_end) = in_edges(*u, hasse);
    for (; ie != ie_end; ++ie) {
      // for each in-edge
      HDOutEdgeIter oe, oe_end;
      std::tie(oe, oe_end) = out_edges(*u, hasse);
      for (; oe != oe_end; ++oe) {
        // for each out-edge
        // source -> u -> target
        HDEdge e;
        bool exists;
        std::tie(e, exists) = edge(
          source(*ie, hasse), target(*oe, hasse), hasse
        );
        
        if (!exists)
          // no transitivity between source and target
          continue;
        
        // remove source -> target, which breaks the no-transitivty rule in
        // the Hasse diagram we need, because a path between source and target
        // already exists in the form: source -> u -> target
        remove_edge(e, hasse);
      }
    }
  }
}

// TODO: delete find_source since it's not used in safe_chain/safe_source?
//       (-> modify source.cpp)
HDVertexIter
find_source(HDVertexIter v, HDVertexIter v_end, const HDGraph& hasse) {
  for (; v != v_end; ++v) {
    if (in_degree(*v, hasse) == 0)
      return v;
  }
  
  return v_end;
}