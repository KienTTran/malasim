#ifndef INTPARASITEDATABASE_H
#define INTPARASITEDATABASE_H

#include <map>
#include <memory>

#include "Utils/TypeDef.h"

class Genotype;
class Config;

using GenotypePtrVector = std::vector<std::unique_ptr<Genotype>>;

class GenotypeDatabase : public GenotypePtrVector {
public:
  // Disallow copy
  GenotypeDatabase(const GenotypeDatabase&) = delete;
  GenotypeDatabase& operator=(const GenotypeDatabase&) = delete;

  // Disallow move
  GenotypeDatabase(GenotypeDatabase&&) = delete;
  GenotypeDatabase& operator=(GenotypeDatabase&&) = delete;

  GenotypeDatabase();

  virtual ~GenotypeDatabase();

  GenotypeDatabase* operator()() { return this; }

  // transfer ownership of genotype to the database
  void add(std::unique_ptr<Genotype> genotype);

  Genotype* get_genotype(const std::string &aa_sequence);

  unsigned int get_id(const std::string &aa_sequence);

  Genotype* get_genotype_from_alleles_structure(const IntVector &alleles);

  double get_min_ec50(int drug_id);

  [[nodiscard]] std::vector<int> get_weight() const { return weight_; }
  void set_weight(const std::vector<int> &value) { weight_ = value; }

  // override at
  Genotype* at(int id) { return GenotypePtrVector::at(id).get(); }

  bool is_matched_genotype(const Genotype* genotype, int chr_id, int gene_id, char aa);
  bool is_matched_genotype_by_id(int genotype_id, int chr_id, int gene_id, char aa);


private:
  std::map<std::string, Genotype*> aa_sequence_id_map_;
  std::map<int, std::map<std::string, double>> drug_id_ec50_;

  unsigned int auto_id_{0};
  std::vector<int> weight_;
};

#endif /* INTPARASITEDATABASE_H */
