#include <algorithm>
#include <cmath>
#include <random>
#include <vector>
#include <map>

template <typename Gene>
struct GeneticAlgorithmTypes
{
  using Individual = std::vector<Gene>;
  using Population = std::vector<Individual>;
  using IndividualPair =
    std::pair<typename GeneticAlgorithmTypes<Gene>::Individual,
              typename GeneticAlgorithmTypes<Gene>::Individual>;
};

class Percentage
{
  double value;

public:
  Percentage(double value)
    : value(value)
  {
    this->value = std::min(1.0, std::max(0.0, this->value));
  }

  operator double() { return this->value; }

  template <typename T>
  double operator*(T x)
  {
    return (double)x * this->value;
  }
};

template <typename T>
double operator*(T x, Percentage p)
{
  return p * (double)x;
}

struct GeneticAlgorithmConfig
{
  GeneticAlgorithmConfig(int population_size, double mutation_rate,
                         double crossover_rate)
    : population_size(population_size)
    , mutation_rate(mutation_rate)
    , crossover_rate(crossover_rate)
  {
  }

  unsigned int population_size;
  Percentage mutation_rate;
  Percentage crossover_rate;
};

template <typename Gene>
struct Mutator
{
  virtual typename GeneticAlgorithmTypes<Gene>::Individual operator()(
    typename GeneticAlgorithmTypes<Gene>::Individual individual);
};

template <typename Gene>
struct Crosser
{
  virtual std::pair<typename GeneticAlgorithmTypes<Gene>::Individual,
                    typename GeneticAlgorithmTypes<Gene>::Individual>
  operator()(typename GeneticAlgorithmTypes<Gene>::Individual individual,
             typename GeneticAlgorithmTypes<Gene>::Individual mate);
};

template <typename Gene>
struct Objective
{
  virtual int operator()(
    typename GeneticAlgorithmTypes<Gene>::Individual individual);
};

struct GaussianMutator : Mutator<double>
{

  GaussianMutator(double sigma)
    : rd()
    , rng(rd())
    , sigma(sigma)
  {
  }

  typename GeneticAlgorithmTypes<double>::Individual operator()(
    typename GeneticAlgorithmTypes<double>::Individual individual)
  {
    typename GeneticAlgorithmTypes<double>::Individual baby;

    for (auto i = individual.begin(); i < individual.end(); i++) {
      std::normal_distribution<double> g(*i, sigma);
      baby.push_back(g(rng));
    }

    return baby;
  }

private:
  double sigma;
  std::random_device rd;
  std::mt19937 rng;
};

template <typename Gene>
struct GenericCrosser : Crosser<Gene>
{
  std::pair<typename GeneticAlgorithmTypes<Gene>::Individual,
                    typename GeneticAlgorithmTypes<Gene>::Individual>
  operator()(typename GeneticAlgorithmTypes<Gene>::Individual individual,
             typename GeneticAlgorithmTypes<Gene>::Individual mate)
  {
    std::uniform_int_distribution<> dist(0, individual.size() - 1);
    auto offset = dist(rng);
    std::swap_ranges(individual.begin(), individual.begin() + offset,
                     mate.begin());
    return std::make_pair(individual, mate);
  }

private:
  std::random_device rd;
  std::mt19937 rng;
};

template <typename Gene>
struct GeneticAlgorithm
{
  using Individual = std::vector<Gene>;
  using Population = std::vector<Individual>;

  GeneticAlgorithm(Individual prototype, GeneticAlgorithmConfig config,
                   Mutator<Gene> mutator, Crosser<Gene> crosser,
                   Objective<Gene> objective)
    : rd()
    , rng(rd())
    , mutator(mutator)
    , crosser(crosser)
    , objective(objective)
    , num_mutate(static_cast<unsigned int>(config.population_size *
                                           config.mutation_rate))
    , num_cross(static_cast<unsigned int>(config.population_size *
                                          config.crossover_rate))
    , population(Population(config.population_size))
  {
    for (unsigned int i = 0; i < config.population_size; i++) {
      population.push_back(prototype);
    }
  }

  void inevitablePassageOfTime()
  {
    using Scores =
      std::map<typename GeneticAlgorithmTypes<Gene>::Individual::iterator,
          int > ;

    /* A simple optimization would be to have two population pools and
     * constantly shift between them instead of reällocating each time
     *
     * This would also help with not needing to rescore
     */
    Population old_population(population);

    Scores old_scores(population.size());

    for (auto i = old_population.begin(); i < old_population.end(); i++) {
      old_scores.insert(Scores::value_type(i, objective(*i)));
    }

    std::shuffle(population.begin(), population.end(), rng);
    std::transform(population.begin(), population.begin() + num_mutate,
                   mutator);
    std::shuffle(population.begin(), population.end(), rng);
    for (auto i = population.begin(); i < population.end() - 1; i += 2) {
      typename GeneticAlgorithmTypes<Gene>::IndividualPair pair =
        crosser(*i, *(i++));
      population.insert(i, pair->first);
      population.insert(i++, pair->second);
    }

    for (auto i = population.begin(); i < population.end(); i++) {
      old_scores.insert(Scores::value_type(i, objective(*i)));
    }
  }

private:
  std::random_device rd;
  std::mt19937 rng;
  Mutator<Gene> mutator;
  Crosser<Gene> crosser;
  Objective<Gene> objective;
  unsigned int num_mutate;
  unsigned int num_cross;
  Population population;
};

struct AllSame : Objective<double>
{
  int operator()(typename GeneticAlgorithmTypes<double>::Individual individual)
  {
    double last = individual.at(0);
    int accum = 0;
    for (auto i = individual.begin(); i < individual.end(); i++) {
      accum += std::ceil(std::abs(last - *i));
    }
    return accum;
  }
};

int
main(int argc, char** argv)
{
  GeneticAlgorithmConfig config = GeneticAlgorithmConfig(100, 0.1, 0.2);
  typename GeneticAlgorithmTypes<double>::Individual prototype = { 10, 20, 30 };

  GeneticAlgorithm<double>(prototype, config, GaussianMutator(0.5),
                           GenericCrosser<double>(), AllSame());
}
