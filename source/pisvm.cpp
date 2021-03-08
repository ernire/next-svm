//
// Created by Ernir Erlingsson on 17.10.2018.
//

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include "pisvm.h"

#define Malloc(type,n) (type *)malloc((n)*sizeof(type))

const char *svm_type_table[] =
        {
                "c_svc","nu_svc","one_class","epsilon_svr","nu_svr", nullptr
        };

const char *kernel_type_table[]=
        {
                "linear","polynomial","rbf","sigmoid", nullptr
        };

#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err34-c"
svm_model *svm_load_model(const char *model_file_name)
{
    FILE *fp = fopen(model_file_name,"rb");
    if(fp== nullptr) return nullptr;

    // read parameters

    auto *model = Malloc(svm_model,1);
    svm_parameter& param = model->param;
    model->rho = nullptr;
    model->probA = nullptr;
    model->probB = nullptr;
    model->label = nullptr;
    model->nSV = nullptr;

    char cmd[81];
    while(1)
    {
        fscanf(fp,"%80s",cmd);

        if(strcmp(cmd,"svm_type")==0)
        {
            fscanf(fp,"%80s",cmd);
            int i;
            for(i=0;svm_type_table[i];i++)
            {
                if(strcmp(svm_type_table[i],cmd)==0)
                {
                    param.svm_type=i;
                    break;
                }
            }
            if(svm_type_table[i] == nullptr)
            {
                fprintf(stderr,"unknown svm type.\n");
                free(model->rho);
                free(model->label);
                free(model->nSV);
                free(model);
                return nullptr;
            }
        }
        else if(strcmp(cmd,"kernel_type")==0)
        {
            fscanf(fp,"%80s",cmd);
            int i;
            for(i=0;kernel_type_table[i];i++)
            {
                if(strcmp(kernel_type_table[i],cmd)==0)
                {
                    param.kernel_type=i;
                    break;
                }
            }
            if(kernel_type_table[i] == nullptr)
            {
                fprintf(stderr,"unknown kernel function.\n");
                free(model->rho);
                free(model->label);
                free(model->nSV);
                free(model);
                return nullptr;
            }
        }
        else if(strcmp(cmd,"degree")==0)
            fscanf(fp,"%d",&param.degree);
        else if(strcmp(cmd,"gamma")==0)
            fscanf(fp,"%lf",&param.gamma);
        else if(strcmp(cmd,"coef0")==0)
            fscanf(fp,"%lf",&param.coef0);
        else if(strcmp(cmd,"nr_class")==0)
            fscanf(fp,"%d",&model->nr_class);
        else if(strcmp(cmd,"total_sv")==0)
            fscanf(fp,"%d",&model->l);
        else if(strcmp(cmd,"rho")==0)
        {
            int n = model->nr_class * (model->nr_class-1)/2;
            model->rho = Malloc(double,n);
            for(int i=0;i<n;i++)
                fscanf(fp,"%lf",&model->rho[i]);
        }
        else if(strcmp(cmd,"label")==0)
        {
            int n = model->nr_class;
            model->label = Malloc(int,n);
            for(int i=0;i<n;i++)
                fscanf(fp,"%d",&model->label[i]);
        }
        else if(strcmp(cmd,"probA")==0)
        {
            int n = model->nr_class * (model->nr_class-1)/2;
            model->probA = Malloc(double,n);
            for(int i=0;i<n;i++)
                fscanf(fp,"%lf",&model->probA[i]);
        }
        else if(strcmp(cmd,"probB")==0)
        {
            int n = model->nr_class * (model->nr_class-1)/2;
            model->probB = Malloc(double,n);
            for(int i=0;i<n;i++)
                fscanf(fp,"%lf",&model->probB[i]);
        }
        else if(strcmp(cmd,"nr_sv")==0)
        {
            int n = model->nr_class;
            model->nSV = Malloc(int,n);
            for(int i=0;i<n;i++)
                fscanf(fp,"%d",&model->nSV[i]);
        }
        else if(strcmp(cmd,"SV")==0)
        {
            while(true)
            {
                int c = getc(fp);
                if(c==EOF || c=='\n') break;
            }
            break;
        }
        else
        {
            fprintf(stderr,"unknown text in model file\n");
            free(model->rho);
            free(model->label);
            free(model->nSV);
            free(model);
            return nullptr;
        }
    }

    // read sv_coef and SV

    int elements = 0;
    long pos = ftell(fp);

    while(true)
    {
        int c = fgetc(fp);
        switch(c)
        {
            case '\n':
                // count the '-1' element
            case ':':
                ++elements;
                break;
            case EOF:
                goto out;
            default:
                ;
        }
    }
    out:
    fseek(fp,pos,SEEK_SET);

    int m = model->nr_class - 1;
    int l = model->l;
    model->sv_coef = Malloc(double *,m);
    int i;
    for(i=0;i<m;i++)
        model->sv_coef[i] = Malloc(double,l);
    model->SV = Malloc(Xfloat *, l);
    model->nz_sv = Malloc(int *, l);
    model->sv_len = Malloc(int, l);
    Xfloat *x_space= nullptr;
    int *nz_x_space= nullptr;
    if(l>0)
    {
        x_space = Malloc(Xfloat,elements);
        nz_x_space = Malloc(int,elements);
    }
    model->max_idx = 0;
    int j=0;
    for(i=0;i<l;i++)
    {
        model->SV[i] = &x_space[j];
        model->nz_sv[i] = &nz_x_space[j];
        model->sv_len[i] = 0;
        for(int k=0;k<m;k++)
            fscanf(fp,"%lf",&model->sv_coef[k][i]);
        while(1)
        {
            int c;
            do {
                c = getc(fp);
                if(c=='\n') goto out2;
            } while(isspace(c));
            ungetc(c,fp);
            //	  fscanf(fp,"%d:%lf",&nz_x_space[j],&x_space[j]);
            fscanf(fp,"%d:%f",&nz_x_space[j],&x_space[j]);
            --nz_x_space[j]; // we need zero based indices
            ++model->sv_len[i];
            ++j;
        }
        out2:
        if(j>=1 && nz_x_space[j-1]+1 > model->max_idx)
        {
            model->max_idx = nz_x_space[j-1]+1;
        }
    }

    fclose(fp);

    model->free_sv = 1;	// XXX
    return model;
}
#pragma clang diagnostic pop