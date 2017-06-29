#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <sstream>
#include <vector>

struct RandomGenerator {
  std::random_device rd;
  std::mt19937 gen;

  RandomGenerator() : gen(rd()) {}
};

struct VotingDistrictId {
  int id;
  VotingDistrictId(int id) : id(id) {}
  bool operator<(const VotingDistrictId other) const {
    return this->id < other.id;
  }
};
struct LegDistrictId {
  int id;
  LegDistrictId(int id) : id(id){};
};

struct VotingDistrict {
  VotingDistrict(VotingDistrictId voting_district_id,
                 LegDistrictId leg_district_id)
      : voting_district_id(voting_district_id),
        leg_district_id(leg_district_id),
        neighbors(std::vector<VotingDistrictId>()) {}

  VotingDistrictId voting_district_id;
  LegDistrictId leg_district_id;
  int republicans;
  int democrats;
  int other;
  std::vector<VotingDistrictId> neighbors;
};

template <typename Gene> struct GeneticAlgorithmType {
  typedef std::vector<std::shared_ptr<Gene>> Individual;
  typedef std::vector<Individual> Population;
};

template <typename Gene> struct Crosser {
  virtual void operator()(typename GeneticAlgorithmType<Gene>::Individual &a,
                          typename GeneticAlgorithmType<Gene>::Individual &b);
};

template <typename Gene> struct Mutator {
  virtual typename GeneticAlgorithmType<Gene>::Individual
  operator()(typename GeneticAlgorithmType<Gene>::Individual &vdist);
};

template <typename Gene> struct Objective {
  virtual int
  operator()(typename GeneticAlgorithmType<Gene>::Population population);
};

template <typename T> T clamp(T a, T n, T x) {
  return std::max(std::min(a, x), n);
}

struct GeneticAlgorithmConfig {
  unsigned int population_size;
  double mutation_rate;
  double crossover_rate;

  GeneticAlgorithmConfig(unsigned int population_size, double mutation_rate,
                         double crossover_rate)
      : population_size(population_size), mutation_rate(mutation_rate),
        crossover_rate(crossover_rate) {}
};

template <typename Gene> struct GeneticAlgorithm {
  typedef std::vector<std::shared_ptr<Gene>> Individual;
  typedef std::vector<Individual> Population;

  Population population;
  GeneticAlgorithmConfig config;
  unsigned int num_mutate;
  unsigned int num_crossover;
  static thread_local RandomGenerator rng;
  GeneticAlgorithm(Individual prototype, GeneticAlgorithmConfig config,
                   Objective<Gene> objective, Crosser<Gene> crosser,
                   Mutator<Gene> mutator)
      : objective(objective), crosser(crosser), mutator(mutator),
        config(config) {
    for (int i = 0; i < config.population_size; i++) {
      population.push_back(prototype);
    }
    num_mutate = static_cast<unsigned int>(
        std::ceil(config.mutation_rate * population.size()));
    num_crossover = static_cast<unsigned int>(
        std::ceil(config.crossover_rate * population.size()));
  }

  void generation() {
    Population new_pop(population.size());
    std::shuffle(population.begin(), population.end(), rng.gen);
    std::transform(population.begin(), population.begin() + num_mutate, new_pop.begin(), mutator);
    std::shuffle(new_pop.begin(), new_pop.end(), rng.gen);
    for (auto it = new_pop.begin(); it < new_pop.begin() + num_crossover - 1;
         it += 2) {
      crosser(*it, *(it + 1));
    }
  }

private:
  Objective<Gene> objective;
  Crosser<Gene> crosser;
  Mutator<Gene> mutator;
};

struct VotingDistrictMutator : Mutator<VotingDistrict> {
  static thread_local RandomGenerator rng;
  using DistrictLookup =
      std::map<VotingDistrictId, std::shared_ptr<VotingDistrict>>;

  const DistrictLookup vdists;

  VotingDistrictMutator(const DistrictLookup vdists) : vdists(vdists) {}


  typename GeneticAlgorithmType<VotingDistrict>::Individual
  operator()(typename GeneticAlgorithmType<VotingDistrict>::Individual indiv) {
    std::uniform_int_distribution<> iudist(0, indiv.size() - 1);
    auto vdist = indiv.at(iudist(rng.gen));
    std::uniform_int_distribution<> vudist(0, vdist->neighbors.size() - 1);
    auto other_vdist = vdist->neighbors.at(vudist(rng.gen));
    vdist->leg_district_id = vdists.at(other_vdist)->leg_district_id;

    return indiv;
  }
};

template <typename Gene> struct GenericCrosser : Crosser<Gene> {
  static thread_local RandomGenerator rng;

  void operator()(typename GeneticAlgorithmType<Gene>::Individual &a,
                  typename GeneticAlgorithmType<Gene>::Individual &b) {
    std::uniform_int_distribution<> dist(0, a.size() - 1);
    auto offset = dist(rng.gen);
    std::swap_ranges(a.begin() + offset, a.end(), b.begin() + offset);
  }
};

struct VotingDistrictObjective : Objective<VotingDistrict> {
  int operator()(
      typename GeneticAlgorithmType<VotingDistrict>::Population population) {
    return 0;
  }
};

int main(int argc, char **argv) {
  std::string line;
  std::ifstream voting_district_file("voting_districts.tsv");

  using DistrictLookup =
      std::map<VotingDistrictId, std::shared_ptr<VotingDistrict>>;
  auto voting_districts = DistrictLookup();
  auto prototype = GeneticAlgorithm<VotingDistrict>::Individual();
  int line_num = 0;
  while (std::getline(voting_district_file, line)) {
    line_num++;
    std::istringstream iss(line);
    int vot_dist, leg_dist, republicans, democrats, other;
    if (!(iss >> vot_dist >> leg_dist >> republicans >> democrats >> other)) {
      std::cerr << "District File: Line " << line_num << " is invalid"
                << std::endl
                << line << std::endl;
      return -2;
    }

    auto voting_district = std::make_shared<VotingDistrict>(vot_dist, leg_dist);
    voting_district->republicans = republicans;
    voting_district->democrats = democrats;
    voting_district->other = other;
    voting_districts.insert(
        DistrictLookup::value_type(vot_dist, voting_district));
    prototype.push_back(voting_district);
  }

  std::ifstream voting_district_neigbors_file("voting_district_neigbors.tsv");
  line_num = 0;
  while (std::getline(voting_district_neigbors_file, line)) {
    line_num++;
    std::istringstream iss(line);
    int vot_dist, neighbor;
    if (!(iss >> vot_dist >> neighbor)) {
      std::cerr << "Neighbor File: Line " << line_num << " is invalid"
                << std::endl
                << line << std::endl;
      return -3;
    }
    voting_districts.at(vot_dist)->neighbors.push_back(neighbor);
  }

  auto ga = GeneticAlgorithm<VotingDistrict>(
      prototype, GeneticAlgorithmConfig(10, 0.1, 0.5),
      VotingDistrictObjective(),
      GenericCrosser<VotingDistrict>(),
      VotingDistrictMutator(voting_districts));

  ga.generation();
}
