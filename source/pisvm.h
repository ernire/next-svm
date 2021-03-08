//
// Created by Ernir Erlingsson on 17.10.2018.
//

#ifndef NEXT_SVM_PISVM_H
#define NEXT_SVM_PISVM_H

typedef float Xfloat;

struct svm_parameter
{
    int svm_type;
    int kernel_type;
    //   double degree;	/* for poly */
    int degree;	/* for poly */
    double gamma;	/* for poly/rbf/sigmoid */
    double coef0;	/* for poly/sigmoid */

    /* these are for training only */
    double cache_size; /* in MB */
    double eps;	/* stopping criteria */
    double C;	/* for C_SVC, EPSILON_SVR and NU_SVR */
    int nr_weight;		/* for C_SVC */
    int *weight_label;	/* for C_SVC */
    double* weight;		/* for C_SVC */
    double nu;	/* for NU_SVC, ONE_CLASS, and NU_SVR */
    double p;	/* for EPSILON_SVR */
    int shrinking;	/* use the shrinking heuristics */
    int probability; /* do probability estimates */
    int o;
    int q;
};
//
// svm_model
//
struct svm_model
{
    svm_parameter param;	// parameter
    int nr_class;		// number of classes, = 2 in regression/one class svm
    int l;			// total #SV
    Xfloat **SV;
    int **nz_sv;
    int *sv_len;
    int max_idx;
    double **sv_coef;	// coefficients for SVs in decision functions (sv_coef[n-1][l])
    double *rho;		// constants in decision functions (rho[n*(n-1)/2])
    double *probA;          // pariwise probability information
    double *probB;

    // for classification only

    int *label;		// label of each class (label[n])
    int *nSV;		// number of SVs for each class (nSV[n])
    // nSV[0] + nSV[1] + ... + nSV[n-1] = l
    // XXX
    int free_sv;		// 1 if svm_model is created by svm_load_model
    // 0 if svm_model is created by svm_train
};

svm_model *svm_load_model(const char *model_file_name);

#endif //NEXT_SVM_PISVM_H
