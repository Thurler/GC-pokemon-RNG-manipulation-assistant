#pragma once

#include <vector>

#include <QButtonGroup>
#include <QLabel>
#include <QProgressBar>
#include <QSpinBox>
#include <QWizardPage>

class SeedFinderPassPage : public QWizardPage
{
public:
  SeedFinderPassPage(QWidget* parent, const int nbrFoundSeeds, const int rtcErrorMarginSeconds,
                     const bool useWii, const bool usePrecalc);
  ~SeedFinderPassPage();

  int nextId() const override;

  virtual std::vector<int> obtainCriteria() = 0;

  void setSeedFinderDone(const bool seedFinderDone);
  void setSeedFinderProgress(const long nbrSeedsSimulated);
  void setNewUsePrecalc(const bool newUsePrecalc);
  virtual void showSeedFinderProgress(const bool showProgress);

protected:
  QLabel* m_lblSeedFinderStatus;
  QProgressBar* m_pbSeedFinder;
  QWidget* m_inputWidget;
  bool m_useWii = false;
  int m_rtcMarginErrorSeconds = 0;
  bool m_usePrecalc = false;

private:
  bool m_seedFinderDone = false;
};

class SeedFinderPassColosseum : public SeedFinderPassPage
{
public:
  SeedFinderPassColosseum(QWidget* parent, const int nbrFoundSeeds, const int rtcErrorMarginSeconds,
                          const bool useWii, const bool usePrecalc);

  std::vector<int> obtainCriteria() override;

private:
  QButtonGroup* m_playerTeamIndexBtnGroup;
  QButtonGroup* m_playerNameIndexBtnGroup;
};

class SeedFinderPassXD : public SeedFinderPassPage
{
public:
  SeedFinderPassXD(QWidget* parent, const int nbrFoundSeeds, const int rtcErrorMarginSeconds,
                   const bool useWii, const bool usePrecalc);

  std::vector<int> obtainCriteria() override;

private:
  QButtonGroup* m_playerTeamIndexBtnGroup;
  QButtonGroup* m_enemyTeamIndexBtnGroup;
  QSpinBox* m_spnTopLeftPkmnHP;
  QSpinBox* m_spnBottomLeftPkmnHP;
  QSpinBox* m_spnTopRightPkmnHP;
  QSpinBox* m_spnBottomRightPkmnHP;
};
