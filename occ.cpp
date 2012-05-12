/*
 * main.cpp
 *
 *  Created on: Apr 18, 2012
 *      Author: jfusion
 */

#include "ocCore.h"
#include "ocVBMManager.h"
#include "ocSBMManager.h"
#include "ocSearchBase.h"
#include "ocReport.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

int main(int argc, char* argv[]) {
    if (argc <= 1) {
        printf("usage: %s [options] data_file\n", argv[0]);
        return 1;
    }
    time_t  t0, t1;
    t0 = clock();
    ocSBMManager *mgr = new ocSBMManager();
    mgr->initFromCommandLine(argc, argv);
    ocReport *report = new ocReport(mgr);
    report->setSeparator(3);

    if (false) {
        mgr->printBasicStatistics();
        mgr->setRefModel("bottom");
        ocModel *fit = mgr->makeSbModel("ABC:D:A1D:B2D", 1);
        mgr->computeL2Statistics(fit);
        mgr->computeDFStatistics(fit);
        mgr->computeDependentStatistics(fit);
        report->addModel(fit);
        mgr->printFitReport(fit, stdout);
        mgr->makeFitTable(fit);
        report->printResiduals(stdout, fit);
        report->printConditional_DV(stdout, fit, false);

    } else {
        double width;
        if (!mgr->getOptionFloat("optimize-search-width", NULL, &width))
            width = 3.0;

        double levels;
        if (!mgr->getOptionFloat("search-levels", NULL, &levels))
            levels = 3.0;

        mgr->printBasicStatistics();

        mgr->setSearch("sb-full-up");
        mgr->setRefModel("bottom");
        ocModel* start = mgr->getBottomRefModel();
        mgr->computeL2Statistics(start);
        mgr->computeDependentStatistics(start);
        mgr->computeIncrementalAlpha(start);
        start->setAttribute("level", 0.0);
        report->addModel(start);
        int nextID = 0;
        mgr->setSortAttr("information");
        mgr->setSearchDirection(0);
        start->setID(nextID++);

        ocModel **nextModels, **keptModels;
        ocModel **model;
        keptModels = new ocModel*[1];
        keptModels[0] = start;
        long count, levelCount, keptCount, nextCount, foundCount;
        keptCount = 1;
        ocModel** models;
        bool found;
        for (int j=0; j < levels; j++) {
            nextCount = 0;
            nextModels = new ocModel*[keptCount * (int)width];
            levelCount = 0;
            printf("level: %d\t", j+1); fflush(stdout);
            for (int k=0; k < keptCount; k++) {
                models = mgr->getSearch()->search(keptModels[k]);
                count = 0;
                if (models) {
                    for (model = models; *model; model++)
                        count++;
                    levelCount += count;
                    for (int i=0; i < count; i++) {
                        mgr->computeInformationStatistics(models[i]);
                    }
                    ocReport::sort(models, count, mgr->getSortAttr(), ocReport::DESCENDING);
                    foundCount = 0;
                    int i = 0;
                    while ((foundCount < (width < count ? width : count)) && (i < count)) {
                        found = false;
                        for (int n=0; n < nextCount; n++) {
                            if (nextModels[n] == models[i] || nextModels[n]->isEquivalentTo(models[i])) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            foundCount++;
                            nextModels[nextCount++] = models[i];
                            models[i]->setProgenitor(keptModels[k]);
                        }
                        i++;
                    }
                    delete[] models;
                }
            }
            delete[] keptModels;
            keptCount = width < nextCount ? width : nextCount;
            keptModels = new ocModel*[keptCount];
            printf("models: %d\tkept: %d\n", levelCount, keptCount); fflush(stdout);
            ocReport::sort(nextModels, nextCount, mgr->getSortAttr(), ocReport::DESCENDING);
            for (int i=0; i < keptCount; i++) {
                nextModels[i]->setAttribute("level", (double)j+1);
                nextModels[i]->setID(nextID++);
                mgr->computeDFStatistics(nextModels[i]);
                mgr->computeL2Statistics(nextModels[i]);
                mgr->computeIncrementalAlpha(nextModels[i]);
                report->addModel(nextModels[i]);
                keptModels[i] = nextModels[i];
            }
            delete[] nextModels;
        }
        delete[] keptModels;

        report->setAttributes("level$I, h, ddf, lr, alpha, information, aic, bic, incr_alpha, prog_id");
        report->sort("information", ocReport::DESCENDING);
        report->print(stdout);
    }
    delete report;
    delete mgr;
    t1 = clock();
    printf("Elapsed time: %f seconds\n", (float)(t1 - t0)/CLOCKS_PER_SEC);
    return 0;
}