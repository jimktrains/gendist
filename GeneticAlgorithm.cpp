#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

template <typename Gene>
struct Mutator;

template <typename Gene>
struct Crosser;

template <typename Gene>
struct Objective;

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
  unsigned int population_size;
  Percentage mutation_rate;
  Percentage crossover_rate;
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
    /* A simple optimization would be to have two population pools and
     * constantly shift between them instead of reällocating each time
     */
    Population new_population(population.size());

    std::shuffle(population.begin(), population.end(), rng);
    std::transform(population.begin(), population.begin() + num_mutate,
                   mutator);
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

template <typename Gene>
struct Mutator
{
  virtual typename GeneticAlgorithm<Gene>::Individual operator()(
    typename GeneticAlgorithm<Gene>::Individual individual);
};

template <typename Gene>
struct Crosser
{
  virtual std::pair<typename GeneticAlgorithm<Gene>::Individual,
                    typename GeneticAlgorithm<Gene>::Individual>
  operator()(typename GeneticAlgorithm<Gene>::Individual individual,
             typename GeneticAlgorithm<Gene>::Individual mate);
};

template <typename Gene>
struct Objective
{
  virtual int operator()(
    typename GeneticAlgorithm<Gene>::Population population);
};

struct GaussianMutator : Mutator<double>
{

  GaussianMutator(double sigma)
    : rd()
    , rng(rd())
    , sigma(sigma)
  {
  }

  typename GeneticAlgorithm<double>::Individual operator()(
    typename GeneticAlgorithm<double>::Individual individual)
  {
    typename GeneticAlgorithm<double>::Individual baby;

    for (auto i = individual.begin(); i < individual.end(); i++) {
      std::normal_distribution<double> g(*i, sigma);
      baby.push_back(g(rng));
    }
  }

private:
  double sigma;
  std::random_device rd;
  std::mt19937 rng;
};

template <typename Gene>
struct GenericCrosser : Crosser<Gene>
{
  virtual std::pair<typename GeneticAlgorithm<Gene>::Individual,
                    typename GeneticAlgorithm<Gene>::Individual>
  operator()(typename GeneticAlgorithm<Gene>::Individual individual,
             typename GeneticAlgorithm<Gene>::Individual mate);
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
