//#ifdef ENABLE_SMC
#include "SMCReporter.h"
#include <fmt/core.h>
#include "Simulation/Model.h"
#include "Population/Person/Person.h"
#include "Utils/Index/PersonIndexAll.h"
#include "Population/Population.h"
#include "Population/DrugsInBlood.h"
#include "Treatment/Therapies/Drug.h"
#include "Configuration/Config.h"
#include <algorithm>   
#include <random>  
#include <vector>
#include <unordered_set>
#include "Events/Population/SMCEvent.h"
#include "Parasites/Genotype.h"
#include "Utils/Index/PersonIndexAll.h"


SMCReporter::SMCReporter() = default;
SMCReporter::~SMCReporter() = default;

void SMCReporter::initialize(int job_number,
                                        const std::string &path) {
  output_file.open(fmt::format("{}smc_tracking_{}.txt", path, job_number));
}

void SMCReporter::before_run() {}

void SMCReporter::begin_time_step() {}

void SMCReporter::monthly_report() {

}

int SMCReporter::get_first_smc_month() const {
    int first = 13; 


    for (const auto& [time, event_ptr] : Model::get_scheduler()->get_world_events().get_events()) {

        auto* smc = dynamic_cast<SMCEvent*>(event_ptr.get());
        if (!smc) continue;

        first = std::min(first, smc->get_smc_month());
    }

    return first == 13 ? -1 : first;
    }

void SMCReporter::custom_report(){

  // Get all person references
//   auto* all_person_index = Model::get_population()->get_person_index<PersonIndexAll>();

  //std::cout<<"SMCReporter: Custom report for SMC tracking at time step "<<Model::get_scheduler()->current_time()<<std::endl;

  auto* all_person_index = Model::get_population()->all_persons();

  //Ensure the tracked list is initialized once if refresh easch interval is set to false
  if ((Model::get_config()->get_smc_reporter_settings().get_smc_refresh_samples_each_interval()) || Model::get_population()->smc_tracked_person_ids().empty()) {
    
    int num_people_tracked = Model::get_config()->get_smc_reporter_settings().get_smc_reporting_number_of_people_tracked();

    
    double min_age_years = 3.0/12.0;  // retrieve from config later
    double max_age_years = 60.0/12.0; // 5 years actual upper limit

    int smc_month = get_first_smc_month();

    


    // If no SMC months found, return without adjusting max age
    if (smc_month != -1) {

        unsigned current_month = unsigned(date::year_month_day(Model::get_scheduler()->get_calendar_date()).month());

        int month_gap = smc_month - current_month;
        if (month_gap < 0) month_gap += 12; // When SMC starts earlier in the year than tracking date, works if we are operating inside a 12-month cycle

        // Shrink by an extra 1 month to account for cases when SMC is given near end of the month
        month_gap += 1;

        // Adjust maximum allowed age so selected kids remain <5 at SMC
        max_age_years -= (month_gap / 12.0);
    }

    // std::cout << "SMCReporter: Selecting " << num_people_tracked
    //           << " people to track, age range: "
    //           << min_age_years << " to " << max_age_years
    //           << " years." << std::endl;



    

    if (Model::get_config()->get_smc_reporter_settings().get_smc_reporting_track_per_district()) {

      // number of people tracked is per district

      //collect preople by district

      std::unordered_map<int, std::vector<Person*>> district_to_people;
      for (auto &person_ptr : all_person_index->v_person()) {
        double age_in_years = person_ptr->age_in_floating(Model::get_scheduler()->current_time());
        if (age_in_years < min_age_years || age_in_years >= max_age_years) continue;
        int district_id = Model::get_spatial_data()->get_admin_unit("district", person_ptr->get_location());
        district_to_people[district_id].push_back(person_ptr.get());
      }

      std::random_device rd;
      std::mt19937 gen(rd());

      auto& tracked_ids = Model::get_population()->smc_tracked_person_ids();

      // Now, for each district, shuffle and pick people
      for (const auto& kv : district_to_people) {
        auto shuffled_ids = kv.second;
        std::shuffle(shuffled_ids.begin(), shuffled_ids.end(), gen);

        int n = std::min(num_people_tracked, static_cast<int>(shuffled_ids.size()));
        tracked_ids.insert(tracked_ids.end(), shuffled_ids.begin(), shuffled_ids.begin() + n);
      }

    }


    else{

    // number of people tracked is countrywide

    // Gather all person pointers that meet the age criteria
    std::vector<Person*> eligible_people;
    eligible_people.reserve(all_person_index->v_person().size());
    for (auto &person_ptr : all_person_index->v_person()) {
        double age_in_years = person_ptr->age_in_floating(Model::get_scheduler()->current_time());
        if (age_in_years >= min_age_years && age_in_years < max_age_years) {
                 eligible_people.push_back(person_ptr.get());
        }

    }

    // Shuffle randomly
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(eligible_people.begin(), eligible_people.end(), gen);

    // Pick the first N people
    int n = std::min(num_people_tracked, static_cast<int>(eligible_people.size()));

    
    auto& tracked_ids = Model::get_population()->smc_tracked_person_ids();
    tracked_ids.assign(eligible_people.begin(), eligible_people.begin() + n);
    }
  }

  /*
  // comment after getting number of children for burkina faso
  int num_people_tracked = Model::CONFIG->smc_reporting_number_of_people_tracked();

    
  double min_age_years = 0; // hardcoded , retrieve from configuration
  double max_age_years = 5;

            

  // Gather all person UIDs
  std::vector<int> all_ids;
  all_ids.reserve(all_person_index->vPerson().size());
  for (auto* person : all_person_index->vPerson()) {
    double age_in_years = person->age_in_floating();
    if (age_in_years >= min_age_years && age_in_years <= max_age_years) {
          all_ids.push_back(person->get_uid());
       }
  
  }


  // Printing population under 5 at the end of each month
    auto current_date = Model::SCHEDULER->calendar_date;

    // Compute the last day of this month
    auto ymd = date::year_month_day{current_date};
    auto next_month = date::year_month{ymd.year(), ymd.month()} + date::months{1};
    auto first_day_next_month = date::sys_days{next_month / date::day{1}};
    auto last_day_this_month = first_day_next_month - date::days{1};

    // Check if current date equals the last day of the month
    if (current_date == last_day_this_month) {
        std::cout << fmt::format(
            "Total number of people with age under 5: {} in {}",
            all_ids.size(),
            date::format("%Y-%m", current_date)
        ) << std::endl;
    }
}

 // END comment after getting number of children for burkina faso
 */

  // uncomment below for actual function

  


  // Now prepare a fast lookup set (used in both cases)
  const auto& tracked_ids_vec = Model::get_population()->smc_tracked_person_ids();
  std::unordered_set<Person*> tracked_ids(tracked_ids_vec.begin(), tracked_ids_vec.end());


  
  for (auto& person_ptr : all_person_index->v_person()) {

    //if (person->host_state() == Person::DEAD) { continue; } // previous (possibly incorrect code)
    //if (person->host_state() == Person::HostStates::DEAD) { continue; }
    // if (person->age_in_floating() > 10) { continue; }
    

    //output_file << fmt::format("Date: {}, Person ID: {}, Location ID: {}, Age: {:.2f}, SP: {}\n", date::format("%Y\t%m\t%d", Model::SCHEDULER->calendar_date), person->get_uid(), person->location(), person->age_in_floating(), person->drugs_in_blood()->is_drug_in_blood(2)); 


    // all present drugs
    
    //for (const auto &kv_drug : *person->drugs_in_blood()->drugs()) {
      ////output_file << fmt::format("Date: {}, Person ID: {}, Location ID: {}, Age: {:.2f}, Drug ID: {}, Last update value: {}\n", date::format("%Y\t%m\t%d", Model::SCHEDULER->calendar_date), person->get_uid(), person->location(), person->age_in_floating(), kv_drug.first, kv_drug.second->last_update_value());
      ////std::cout<<kv_drug.first<<std::endl;
    //  output_file << fmt::format("Date: {}, Person ID: {}, Location ID: {}, Age: {:.2f}, Drug ID: {}, present: {}\n", date::format("%Y\t%m\t%d", Model::SCHEDULER->calendar_date), person->get_uid(), person->location(), person->age_in_floating(), kv_drug.first, (person->drugs_in_blood()->is_drug_in_blood(kv_drug.first) ? "true" : "false") );

    //}
    

   // only drug 2 (SP) or drug 1 (AQ)

   //uncomment below to track all people

   

    //if (person->drugs_in_blood()->is_drug_in_blood(2)) {
    //  output_file << fmt::format("Date: {}, Person ID: {}, Location ID: {}, Age: {:.2f}, Drug ID: {}, Last update value: {}\n", date::format("%Y\t%m\t%d", Model::SCHEDULER->calendar_date), person->get_uid(), person->location(), person->age_in_floating(), 2, person->drugs_in_blood()->get_drug(2)->last_update_value());
    //}

    //if (person->drugs_in_blood()->is_drug_in_blood(1)) {
    //  output_file << fmt::format("Date: {}, Person ID: {}, Location ID: {}, Age: {:.2f}, Drug ID: {}, Last update value: {}\n", date::format("%Y\t%m\t%d", Model::SCHEDULER->calendar_date), person->get_uid(), person->location(), person->age_in_floating(), 1, person->drugs_in_blood()->get_drug(1)->last_update_value());
    //}


    // end track all people

  

    // keeping track of infection and concentration of drug in bloof of people selected

    if (tracked_ids.count(person_ptr.get()) > 0) {
        // Get infection state as string
        std::string state_str;
        switch (person_ptr->get_host_state()) {
            case Person::HostStates::SUSCEPTIBLE: state_str = "SUSCEPTIBLE"; break;
            case Person::HostStates::EXPOSED: state_str = "EXPOSED"; break;
            case Person::HostStates::ASYMPTOMATIC: state_str = "ASYMPTOMATIC"; break;
            case Person::HostStates::CLINICAL: state_str = "CLINICAL"; break;
            case Person::HostStates::DEAD: state_str = "DEAD"; break;
            default: state_str = "UNKNOWN"; break;
        }

        

        // get parasite density (reference : IndividaulsFileReporter)
        double p_density;
        if (person_ptr->get_all_clonal_parasite_populations()->size() >= 1) {
        p_density = person_ptr->get_all_clonal_parasite_populations()->at(0)->last_update_log10_parasite_density();
      } else {
        p_density =
            Model::get_config()->get_parasite_parameters().get_parasite_density_levels().get_log_parasite_density_cured();
      }

      const std::size_t parasite_population_size = person_ptr->get_all_clonal_parasite_populations()->size();

      // Build genotype summary for all clonal parasite populations
      std::ostringstream genotype_stream;

      for (std::size_t j = 0ul; j < parasite_population_size; j++) {
          ClonalParasitePopulation* bp =
              person_ptr->get_all_clonal_parasite_populations()->at(j);

          if (j > 0) {
              genotype_stream << "; ";
          }

          genotype_stream << bp->genotype()->genotype_id()
                          << "(" << bp->last_update_log10_parasite_density() << ")";

        //   std::cout << "Parasite population " << j
        //             << ": Genotype ID = " << bp->genotype()->genotype_id()
        //             << ", Log10 Parasite Density = " << bp->last_update_log10_parasite_density()
        //             << std::endl;
      }
      // std::cout<<std::endl;

      std::string genotypes = genotype_stream.str();


        // Get drug concentrations if present
        double drug0_val = 0.0, drug1_val = 0.0, drug2_val = 0.0, drug4_val = 0.0; // 0 => artemisinin, 1 => amodiaquine, 2 => SP, 4 => lumefantrine

        for (const auto &[drug_id, drug] : *person_ptr->drugs_in_blood()) {
            if (drug == nullptr) { continue; }

            if (drug_id == 0) { drug0_val = drug->last_update_value(); }
            else if (drug_id == 1) { drug1_val = drug->last_update_value(); }
            else if (drug_id == 2) { drug2_val = drug->last_update_value(); }
            else if (drug_id == 4) { drug4_val = drug->last_update_value(); }


        }






        int district_id = Model::get_spatial_data()->get_admin_unit("district", person_ptr->get_location());

        // Write one combined log line
        output_file << fmt::format(
            "Date: {}, ID: {}, Loc: {}, Age: {:.2f}, State: {}, Drug0: {:.4f}, Drug1: {:.4f}, Drug2: {:.4f}, Drug3: {:.4f}, ParasiteDensity: {:.4f}, Genotypes: {}\n",
            date::format("%Y-%m-%d", Model::get_scheduler()->get_calendar_date()),
            static_cast<const void*>(person_ptr.get()),
            district_id,
            person_ptr->age_in_floating(Model::get_scheduler()->current_time()),
            state_str,
            drug0_val,
            drug1_val,
            drug2_val,
            drug4_val,
            p_density,
            genotypes
        );



    

    }
  }


}



void SMCReporter::after_run() {
   output_file << fmt::format("SMC Reporter After Run");

  // close the file
  if (output_file.is_open()) { output_file.close(); }
}



//#endif  // ENABLE_SMC
