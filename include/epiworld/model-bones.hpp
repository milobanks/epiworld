#include <vector>
#include <random>
#include <unordered_map>
#include "misc.hpp"

#ifndef EPIWORLD_MODEL_HPP
#define EPIWORLD_MODEL_HPP

template<typename TSeq>
class Person;

template<typename TSeq>
class Virus;

template<typename TSeq>
class Tool;

class AdjList;

template<typename TSeq>
class DataBase;

template<typename TSeq>
class Model {
    friend class Person<TSeq>;
    friend class DataBase<TSeq>;
private:

    DataBase<TSeq> db;

    std::vector< Person<TSeq> > population;
    std::map< int,int >         population_ids;
    bool directed;
    
    std::vector< Virus<TSeq> > viruses;
    std::vector< double > prevalence_virus; ///< Initial prevalence_virus of each virus
    
    std::vector< Tool<TSeq> > tools;
    std::vector< double > prevalence_tool;

    std::shared_ptr< std::mt19937 > engine =
        std::make_shared< std::mt19937 >();
    
    std::shared_ptr< std::uniform_real_distribution<> > runifd =
        std::make_shared< std::uniform_real_distribution<> >(0.0, 1.0);

    std::shared_ptr< std::normal_distribution<> > rnormd =
        std::make_shared< std::normal_distribution<> >(0.0);

    std::shared_ptr< std::gamma_distribution<> > rgammad = 
        std::make_shared< std::gamma_distribution<> >();

    std::function<void(std::vector<Person<TSeq>>*,Model<TSeq>*,double)> rewire_fun;
    double rewire_prop;
        
    std::map<std::string, double > parameters;
    unsigned int ndays;
    Progress pb;

    std::vector< unsigned int > status_susceptible = {STATUS::HEALTHY, STATUS::RECOVERED};
    std::vector< std::string > status_susceptible_labels = {"healthy", "recovered"};
    std::vector< unsigned int > status_infected = {STATUS::INFECTED};
    std::vector< std::string > status_infected_labels = {"infected"};
    std::vector< unsigned int > status_removed = {STATUS::REMOVED};
    std::vector< std::string > status_removed_labels = {"removed"};

    unsigned int nstatus = 4u;
    
    bool verbose     = true;
    bool initialized = false;
    int current_date = 0;

    void dist_tools();
    void dist_virus();

    std::chrono::time_point<std::chrono::steady_clock> time_start;
    std::chrono::time_point<std::chrono::steady_clock> time_end;

    // std::chrono::milliseconds
    std::chrono::duration<double,std::micro> time_elapsed = 
        std::chrono::duration<double,std::micro>::zero();
    unsigned int time_n = 0u;
    void chrono_start();
    void chrono_end();

    std::unique_ptr< Model<TSeq> > backup = nullptr;

    UpdateFun<TSeq> update_susceptible = nullptr;
    UpdateFun<TSeq> update_infected = nullptr;
    UpdateFun<TSeq> update_removed = nullptr;

public:

    Model() {};
    Model(const Model<TSeq> & m);
    Model(Model<TSeq> && m) = delete;
    Model<TSeq> & operator=(const Model<TSeq> & m) = delete;

    void clone_population(
        std::vector< Person<TSeq> > & p,
        std::map<int,int> & p_ids,
        bool & d,
        Model<TSeq> * m = nullptr
    ) const ;

    /**
     * @brief Set the backup object
     * @details `backup` can be used to restore the entire object
     * after a run. This can be useful if the user wishes to have
     * individuals start with the same network from the beginning.
     * 
     */
    ///@[
    void set_backup();
    void restore_backup();
    ///@]

    DataBase<TSeq> & get_db();
    double & operator()(std::string pname);

    size_t size() const;

    /**
     * @brief Random number generation
     * 
     * @param eng 
     */
    ///@[
    void set_rand_engine(std::mt19937 & eng);
    std::mt19937 * get_rand_endgine();
    void seed(unsigned int s);
    void set_rand_gamma(double alpha, double beta);
    double runif();
    double rnorm();
    double rnorm(double mean, double sd);
    double rgamma();
    double rgamma(double alpha, double beta);
    ///@]

    void add_virus(Virus<TSeq> v, double preval);
    void add_tool(Tool<TSeq> t, double preval);

    /**
     * @brief Accessing population of the model
     * 
     * @param fn std::string Filename of the edgelist file.
     * @param skip int Number of lines to skip in `fn`.
     * @param directed bool Whether the graph is directed or not.
     * @param min_id int Minimum id number (if negative, the program will
     * try to guess from the data.)
     * @param max_id int Maximum id number (if negative, the program will
     * try to guess from the data.)
     * @param al AdjList to read into the model.
     */
    ///@[
    void pop_from_adjlist(
        std::string fn,
        int skip = 0,
        bool directed = false,
        int min_id = -1,
        int max_id = -1
        );
    void pop_from_adjlist(AdjList al);
    bool is_directed() const;
    std::vector< Person<TSeq> > * get_population();
    ///@]

    /**
     * @brief Functions to run the model
     * 
     * @param seed Seed to be used for Pseudo-RNG.
     * @param ndays Number of days (steps) of the simulation.
     * 
     */
    ///@[
    void init(unsigned int seed, unsigned int ndays);
    void update_status();
    void mutate_variant();
    void next();
    void run();
    ///@]

    void record_variant(Virus<TSeq> * v);

    int get_nvariants() const;
    unsigned int get_ndays() const;
    void set_ndays(unsigned int ndays);
    bool get_verbose() const;
    void verbose_off();
    void verbose_on();
    int today() const;

    /**
     * @brief Rewire the network preserving the degree sequence.
     *
     * @details This implementation assumes an undirected network,
     * thus if {(i,j), (k,l)} -> {(i,l), (k,j)}, the reciprocal
     * is also true, i.e., {(j,i), (l,k)} -> {(j,k), (l,i)}.
     * 
     * @param proportion Proportion of ties to be rewired.
     * 
     * @result A rewired version of the network.
     */
    ///@[
    void set_rewire_fun(std::function<void(std::vector<Person<TSeq>>*,Model<TSeq>*,double)> fun);
    void set_rewire_prop(double prop);
    double get_rewire_prop() const;
    void rewire();
    ///@]

    inline void set_update_susceptible(UpdateFun<TSeq> fun);
    inline void set_update_infected(UpdateFun<TSeq> fun);
    inline void set_update_removed(UpdateFun<TSeq> fun);
    /**
     * @brief Wrapper of `DataBase::write_data`
     * 
     * @param fn_variant_info 
     * @param fn_variant_hist 
     * @param fn_total_hist 
     * @param fn_transmission 
     */
    void write_data(
        std::string fn_variant_info,
        std::string fn_variant_hist,
        std::string fn_total_hist,
        std::string fn_transmission
        ) const;

    /**
     * @brief Export the network data in edgelist form
     * 
     * @param fn std::string. File name.
     * @param source Integer vector
     * @param target Integer vector
     * 
     * @details When passing the source and target, the function will
     * write the edgelist on those.
     */
    ///[@
    void write_edgelist(
        std::string fn
        ) const;

    void write_edgelist(
        std::vector< unsigned int > & source,
        std::vector< unsigned int > & target
        ) const;
    ///@]

    std::map<std::string, double> & params();

    /**
     * @brief Reset the model
     * 
     * @details Resetting the model will:
     * - clear the database
     * - restore the population (if `set_backup()` was called before)
     * - re-distribute tools
     * - re-distribute viruses
     * - set the date to 0
     * 
     */
    void reset();
    void print() const;

    Model<TSeq> && clone() const;

    /**
     * @brief Adds extra statuses to the model
     * 
     * @details
     * Adding values of `s` that are already present in the model will
     * result in an error.
     * 
     * The functions `get_status_*` return the current values for the 
     * statuses included in the model.
     * 
     * @param s `unsigned int` Code of the status
     * @param lab `std::string` Name of the status.
     * 
     * @return `add_status*` returns nothing.
     * @return `get_status_*` returns a vector of pairs with the 
     * statuses and their labels.
     */
    ///@[
    void add_status_susceptible(unsigned int s, std::string lab);
    void add_status_infected(unsigned int s, std::string lab);
    void add_status_removed(unsigned int s, std::string lab);
    void add_status_susceptible(std::string lab);
    void add_status_infected(std::string lab);
    void add_status_removed(std::string lab);
    const std::vector< unsigned int > & get_status_susceptible() const;
    const std::vector< unsigned int > & get_status_infected() const;
    const std::vector< unsigned int > & get_status_removed() const;
    const std::vector< std::string > & get_status_susceptible_labels() const;
    const std::vector< std::string > & get_status_infected_labels() const;
    const std::vector< std::string > & get_status_removed_labels() const;
    ///@]

    ///@[
    double add_param(double initial_val, std::string pname);
    double set_param(std::string pname);
    double get_param(unsigned int k);
    double get_param(std::string pname);
    double par(unsigned int k);
    double par(std::string pname);
    double * p00,*p01,*p02,*p03,*p04,*p05,*p06,*p07,*p08,*p09,*p10;
    unsigned int npar_used = 0u;
    ///@]

    void get_elapsed(
        std::string unit = "auto",
        double * last_elapsed = nullptr,
        double * total_elapsed = nullptr,
        unsigned int * n_replicates = nullptr,
        std::string * unit_abbr = nullptr,
        bool print = true
    ) const;

};

#endif