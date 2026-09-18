#include <gtest/gtest.h>

#include "Configuration/GenotypeParameters.h"

TEST(PfGenotypeInfoCoverageTest, CalculatesAminoAcidOffsetsAndTracksSelectingCnvDrugs) {
  GenotypeParameters::PfGenotypeInfo info;
  GenotypeParameters::GeneInfo regular;
  regular.set_name("regular");
  regular.set_max_copies(1);
  regular.set_aa_positions({GenotypeParameters::AminoAcidPosition(),
                            GenotypeParameters::AminoAcidPosition()});

  GenotypeParameters::GeneInfo cnv;
  cnv.set_name("cnv");
  cnv.set_max_copies(2);
  GenotypeParameters::MultiplicativeEffectOnEC50 effect;
  effect.set_drug_id(4);
  effect.set_factors({1.0, 2.0});
  cnv.set_cnv_multiplicative_effect_on_EC50({effect});
  cnv.set_aa_positions({GenotypeParameters::AminoAcidPosition()});

  info.chromosome_infos[0].set_chromosome_id(0);
  info.chromosome_infos[0].set_genes({regular, cnv});
  info.chromosome_infos[1].set_chromosome_id(1);
  info.chromosome_infos[1].set_genes({cnv});
  info.refresh_cnv_gene_indices();

  ASSERT_EQ(info.get_cnv_gene_indices().size(), 2u);
  EXPECT_EQ(info.get_cnv_gene_indices()[0].selecting_drug_ids, std::vector<int>({4}));
  EXPECT_EQ(info.calculate_aa_pos(1, 0, 0), 6);
  EXPECT_NE(info.to_string().find("cnv"), std::string::npos);
}

TEST(PfGenotypeInfoCoverageTest, IgnoresNonSelectingAndShortCnvFactors) {
  GenotypeParameters::PfGenotypeInfo info;
  GenotypeParameters::GeneInfo cnv;
  cnv.set_max_copies(2);
  GenotypeParameters::MultiplicativeEffectOnEC50 effect;
  effect.set_drug_id(1);
  effect.set_factors({1.0});
  cnv.set_cnv_multiplicative_effect_on_EC50({effect});
  info.chromosome_infos[0].set_genes({cnv});
  info.refresh_cnv_gene_indices();
  ASSERT_EQ(info.get_cnv_gene_indices().size(), 1u);
  EXPECT_TRUE(info.get_cnv_gene_indices()[0].selecting_drug_ids.empty());
}
