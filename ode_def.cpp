
/* ============================================================================
 File   : ode_def.cpp
 Author : Sylvie Putot, Ecole Polytechnique (France)
 
 Part of the RINO package for Inner and Outer Reachability Analysis.
 
The place to define initial conditions and parameters the systems of ODEs or DDEs on which to perform reachability
============================================================================ */

#include <assert.h>
#include <math.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_sf_gamma.h>

#include "filib_interval.h"
#include "tadiff.h" 
#include "fadiff.h"
#include "fadbad_aa.h"
#include "ode_def.h"
#include "matrix.h"

using namespace std;

int sysdim; // dimension of system of ODE/DDE
int jacdim;  //  Jacobian will be dimension sysdim * jacdim, for ODEs jacdim = sysdim + sysdim_jacparams
int sysdim_params;
int sysdim_jacparams; // params that appear in the Jacobian but are not defined as solutions of ODE

double t_end; // ending time of integration
double t_begin; // starting time of initialization

// parameters of the system of the ODEs
vector<AAF> params;  // params of the ODE (nondeterministic disturbances)
vector<AAF> inputs; // uncertain inputs and parameters : some will be used in initial condition, some as uncertain parameters
vector<AAF> center_inputs;
vector<int> index_param;
vector<int> index_param_inv;
vector<interval> eps;

// for subdivisions of the initial domain to refine precision
int nb_subdiv_init = 1; // number of subdivisiions
int component_to_subdiv = 0;

double recovering = 0.0; // percentage of recovering between subdivisions
vector<vector<vector<interval>>> Xouter_print, Xouter_robust_print, Xouter_minimal_print, Xinner_print, Xinner_joint_print, Xinner_robust_print, Xinner_minimal_print, Xexact_print; // store results of subdivision
vector<double> t_print; // times where results are stored
int current_subdiv;
int current_iteration;

// for robust inner-approximations
int uncontrolled; // number of uncontrolled parameters (forall params)
int controlled; // number of controlled parameters (forall params)
vector<bool> is_uncontrolled; // for each input, uncontrolled or controlled (for robust inner-approx)
vector<bool> is_initialcondition; // for each input, initial condition or parameter (for robust outer-approx)
int variable; // number of non constant parameters
vector<bool> is_variable;  // for each parameter, constant or variable
vector<int> nb_inputs; // piecewise constant input changes value every t_end/nb_inputs[i] seconds

bool refined_mean_value;



// define the dimensions of your system (ODE or DDE) and if we want initial subdivisions
void define_system_dim(int argc, char* argv[])
{
    /*************************************************************************** ODE ************************************************************/
    
    sysdim_params = 0;
    sysdim_jacparams = 0;
    nb_subdiv_init = 1; // nb of initial subdivisions of the input range

    if (systype == 0) // ODE
    {
      
        if (syschoice == 1)  // running example
        {
            sysdim = 1;
            jacdim = 1;
            sysdim_params = 1;
        }
        else if (syschoice == 2)  // Brusselator
        {
            sysdim = 2;
            jacdim = 2;
            sysdim_params = 2;
        }
        else if (syschoice == 3)  // ballistic
        {
            sysdim = 4;
            jacdim = 4;
        }
        else if (syschoice == 4)  // ballistic linearise + masse incertaine
        {
            sysdim = 4;
            sysdim_jacparams = 1;
            jacdim = sysdim+sysdim_jacparams;
        }
        else if (syschoice == 5)  // self-driving car
        {
            sysdim = 2;
            jacdim = 2;
            sysdim_params = 2;
        }
     /*   else if (syschoice == 6)  //  self-driving car
        {
            sysdim = 2;
            jacdim = 4;
       //     sysdim_params = 2;
        } */
        else if (syschoice == 6)  //  self-driving car
        {
            sysdim = 2;
            sysdim_jacparams = 2;
            jacdim = sysdim+sysdim_jacparams;
            //     sysdim_params = 2;
        }
        else if (syschoice == 7)  //  self-driving car
        {
            sysdim = 4;
            jacdim = 4;
            //     sysdim_params = 2;
        }
        else if (syschoice == 8)
        {
            sysdim = 1;
            jacdim = 1;
        }
        else if (syschoice == 11)  // academic example to investigate time-varying parameters
        {
            sysdim = 2;
            jacdim = 4;
        }
        else if (syschoice == 12)  // academic example to investigate time-varying parameters
        {
            sysdim = 4;
            jacdim = 4;
        }
        else if (syschoice == 13)  // Laub-Loomis Benchmark [Arch 2019]
        {
            sysdim = 7;
            jacdim = 7;
            sysdim_params = 1;
        }
        else if (syschoice == 14) // Van der Pol oscillator [Arch 2019]
        {
            sysdim = 2;
            jacdim = 2;
        }
        else if (syschoice == 15) // Van der Pol oscillator [Arch 2018 and Sparse Polynomial zonotopes]
        {
            sysdim = 2;
            jacdim = 2;
        }
        else if(syschoice == 17) // quadrotor model [Arch 2019]
        {
            sysdim = 12;
            jacdim = 12;
        }
        else if (syschoice == 18) // HSCC 2019 paper crazyflie example
        {
            sysdim = 14;
            jacdim = 14;
            // sysdim_params = 3;
            // 0 for sysdim params
        }
        else if (syschoice == 19) {  // academic example, time-varying (piecewise constant) parameters
            sysdim = 2;
            sysdim_jacparams = 1;
            jacdim = sysdim+sysdim_jacparams;
        }
        else if (syschoice == 21) {  // academic example, time-varying (piecewise constant) parameters
            sysdim = 2;
            sysdim_jacparams = 1;
            jacdim = sysdim + sysdim_jacparams;
            
        }
        else if (syschoice == 22) {  // academic example, time-varying (piecewise constant) parameters
            sysdim = 2;
            sysdim_jacparams = 1;
            jacdim = sysdim + sysdim_jacparams;
        }
        
        
        // jacdim = 4
    }
    /*************************************************************************** DDE ************************************************************/
    else if (systype == 1) // DDE
    {
        
        if (syschoice == 1)  // running example
        {
            sysdim = 1;
            jacdim = 1;
      //      nb_subdiv_init = 2;
      //      nb_subdiv_init = 10;
      //      recovering = 0.1; // recovering between 2 subdivisions when subdividing initial parameters
            
        }
        if (syschoice == 2)  //
        {
            sysdim = 2;
            jacdim = 2;
        }
        else if (syschoice == 3) // Xue 2017 (Ex 3)
        {
            sysdim = 7;
            jacdim = 7;
        }
        else if (syschoice == 4) // Szczelina 1 and 2 2014
        {
            sysdim = 1;
            jacdim = 1;
        }
        else if (syschoice == 5) // Szczelina 2 2014
        {
            sysdim = 1;
            jacdim = 1;
        }
        else if (syschoice == 6) // self-driving car
        {
            sysdim = 2;
            jacdim = 2;
            sysdim_params = 2;
        }
        else if (syschoice == 7) // self-driving car but with coeff in interv
        {
            sysdim = 4;
            jacdim = 4;
        }
        else if (syschoice == 8) // self-driving car but with coeff in interv
        {
            sysdim = 2;
            sysdim_jacparams = 2;
            jacdim = 4;
        }
        else if (syschoice == 9) 
        {
            sysdim = 1;
            jacdim = 1;
        }
        else if (syschoice == 10) // platoon of 5 vehicles
        {
            sysdim = 9;
            jacdim = 9;
        }
        else if (syschoice == 11) // platoon of 7 vehicles
        {
            sysdim = 19;
            jacdim = 19;
        }
    }
    
    if (argc == 4) // called with configuration file: we overwrite the initialization of init_system
        readfromfile_system_dim(argv[3], sysdim, jacdim, sysdim_params, nb_subdiv_init);
    
}


// for ODEs : initialize the state variable (and center for inner-approximation)
void set_initialconditions(vector<AAF> &param_inputs, vector<AAF> &param_inputs_center, vector<AAF> &x, vector<AAF> &xcenter, vector<vector<AAF>> &J)
{
    assert(systype == 0); // ODE
   
    // par défaut
    for (int i=0 ; i<sysdim ; i++) {
        x[i] = inputs[i];
        xcenter[i] = center_inputs[i];
    }
    
    // param_inputs between 0 and jacdim-sysdim, inputs between 0 and sysdim_jacparams
        int j = 0;
        for (int i = 0; i < sysdim_jacparams ; i++) {
            for (int k=0; k<nb_inputs[i] ; k++) {
                index_param[j] = i;
                if (k == 0)
                    index_param_inv[i] = j;
                param_inputs[j] = inputs[sysdim+j]; // inputs[sysdim+i].convert_int(); // create a new independent AAF ?
                if (innerapprox == 1)
                    param_inputs_center[j] = center_inputs[sysdim+j]; // center_inputs[sysdim+i].convert_int();
                else
                    param_inputs_center[j] = inputs[sysdim+j]; // inputs[sysdim+i].convert_int();
                j++;
            }
        }
    
    setId(J);
    
    
    
 /*   if (syschoice == 6) // self-driving car; sysdim = 2, jacdim = 4
    {
        x[0] = inputs[0];
        x[1] = inputs[1];
        xcenter[0] = center_inputs[0];
        xcenter[1] = center_inputs[1];
        J[0][0] = 1.;
        for (int i=1 ; i<=3 ; i++)
            J[0][i] = 0.;
        J[1][0] = 0.0;
        J[1][1] = 1.;
        for (int i=2 ; i<=3 ; i++)
            J[1][i] = 0.;
    }
   */
    
}

void readfromfile_system_dim(const char * params_filename, int &sysdim, int &jacdim, int &sysdim_params, int &nb_subdiv_init)
{
    const int LINESZ = 2048;
    char buff[LINESZ];
    
    cout << "****** Reading system dimensions from file " <<  params_filename << " ******" << endl;
    FILE *params_file = fopen(params_filename,"r");
    if (params_file == NULL)
        cout << "Error reading " << params_filename << ": file not found" << endl;
    while (fgets(buff,LINESZ,params_file)) {
        sscanf(buff, "system-dimension = %d\n", &sysdim);
        sscanf(buff, "jacobian-dimension = %d\n", &jacdim);
        sscanf(buff, "sys-parameters-dimension = %d\n", &sysdim_params);
        sscanf(buff, "nb-initial-subdivisions = %d\n", &nb_subdiv_init);
    }
    fclose(params_file);
}

// d0 and t_begin are for DDEs only, rest are common to ODE and DDE
void read_parameters(const char * params_filename, double &tau, double &t_end, double &d0, double &t_begin, int &order, int &nb_subdiv)
{
    const int LINESZ = 2048;
    char buff[LINESZ];
    char initialcondition[LINESZ];
    const char space[2] = " ";
    double a, b;
    
    cout << "****** Reading system parameter from file " <<  params_filename << " ******" << endl;
    FILE *params_file = fopen(params_filename,"r");
    if (params_file == NULL)
        cout << "Error reading " << params_filename << ": file not found" << endl;
    
    
    
    while (fgets(buff,LINESZ,params_file)) {
        //     sscanf(buff, "system = %s\n", sys_name);
        //      sscanf(buff, "initially = %[^\n]\n", initial_condition);   // tell separator is newline, otherwise by default it is white space
        sscanf(buff, "time-horizon = %lf\n", &t_end);
        sscanf(buff, "sampling-time = %lf\n", &tau);
        //      sscanf(buff, "output-variables = %[^\n]\n", output_variables);
        sscanf(buff, "delay = %lf\n", &d0);               // for DDEs
        sscanf(buff, "starting-time = %lf\n", &t_begin);  // for DDEs
        sscanf(buff, "nb-time-subdivisions = %d\n", &nb_subdiv); // for DDEs : subdiv of time interval d0 : tau is deduced
        sscanf(buff, "order = %d\n", &order);
        sscanf(buff, "interactive-visualization = %d\n", &interactive_visualization);
        if (sscanf(buff, "variables-to-display = %s\n", initialcondition) == 1)
        {
            for (int i=0; i< sysdim; i++)
                variables_to_display[i] = false;
            
            char *token = strtok(buff,space);
            token = strtok(NULL,space);
            token = strtok(NULL,space);
            int i;
            while( token != NULL ) {
                sscanf(token,"%d",&i);
                variables_to_display[i] = true;
             //   cout <<"input="<<inputs[i].convert_int()<<endl;
                token = strtok(NULL,space);
            }
        }
        if (sscanf(buff, "inputs = %s\n", initialcondition) == 1)
        {
            char *token = strtok(buff,space);
            token = strtok(NULL,space);
            token = strtok(NULL,space);
            int i = 0;
            while( token != NULL ) {
                sscanf(token,"[%lf,%lf]",&a,&b);
                inputs[i] = interval(a,b);
               // cout <<"input="<<inputs[i].convert_int()<<endl;
                i++;
                token = strtok(NULL,space);
            }
        }
        if (sscanf(buff, "params = %s\n", initialcondition) == 1)
        {
            char *token = strtok(buff,space);
            token = strtok(NULL,space);
            token = strtok(NULL,space);
            int i = 0;
            while( token != NULL ) {
                sscanf(token,"[%lf,%lf]",&a,&b);
                params[i] = interval(a,b);
                  //       cout <<"params="<<params[i].convert_int()<<endl;
                i++;
                token = strtok(NULL,space);
            }
        }
        if (sscanf(buff, "uncontrolled = %s\n", initialcondition) == 1)
        {
            for (int i=0 ; i<jacdim; i++)
                is_uncontrolled[i] = false;
                
            char *token = strtok(buff,space);
            token = strtok(NULL,space);
            token = strtok(NULL,space);
            int i;
            while( token != NULL ) {
                sscanf(token,"%d",&i);
                is_uncontrolled[i] = true;
          //      cout <<"is_uncontrolled="<<i<<endl;
                token = strtok(NULL,space);
            }
        }
        if (sscanf(buff, "variable = %s\n", initialcondition) == 1)
        {
            for (int i=0 ; i<jacdim; i++)
                is_variable[i] = false;
                
            char *token = strtok(buff,space);
            token = strtok(NULL,space);
            token = strtok(NULL,space);
            int i;
            while( token != NULL ) {
                sscanf(token,"%d",&i);
                is_variable[i] = true;
              //  cout <<"is_variable="<<i<<endl;
                token = strtok(NULL,space);
            }
        }
        if (sscanf(buff, "initial-condition = %s\n", initialcondition) == 1)
        {
            for (int i=0 ; i<jacdim; i++)
                is_initialcondition[i] = false;
            
            char *token = strtok(buff,space);
            token = strtok(NULL,space);
            token = strtok(NULL,space);
            int i;
            while( token != NULL ) {
                sscanf(token,"%d",&i);
                is_initialcondition[i] = true;
                //  cout <<"is_variable="<<i<<endl;
                token = strtok(NULL,space);
            }
        }
    }
    fclose(params_file);
    
    // cout << "system name = " << sys_name << endl;
      cout << "****** End parameter reading ******" << endl << endl;
}



// the main function to define the system
// for ODEs and DDEs: define bounds for parameters and inputs, value of delay d0 if any, and parameters of integration (timestep, order of TM)
void init_system(int argc, char* argv[], double &t_begin, double &t_end, double &tau, double &d0, int &nb_subdiv, int &order /*, vector<interval> &ix*/)
{
    interval temp;
    int nb_points;
    
    define_system_dim(argc,argv); // defines value of sysdim: depends on syschoice -- reads from file if input at command-line
    
    // ******* for variable inputs ********
    nb_inputs = vector<int>(sysdim_jacparams);
    for (int i=0; i<sysdim_jacparams; i++)
        nb_inputs[i] = 1;
    
    if (systype == 0) // ODE
    {
        t_begin = 0;
        if (syschoice == 21)  // running example
        {
            nb_inputs[0] = 2; // piecewise constant input changes value every t_end/nb_inputs[i] seconds
        }
        else if (syschoice == 22)  // running example
        {
            nb_inputs[0] = 2; // piecewise constant input changes value every t_end/nb_inputs[i] seconds
        }
    }
    for (int i=0; i<sysdim_jacparams; i++)
        jacdim += nb_inputs[i]-1;
    // correspondance between variable inputs wwhich sucessive bounds are stored in inputs of size [jacdim]
    index_param = vector<int>(jacdim-sysdim);
    index_param_inv = vector<int>(sysdim_jacparams);
    int j = 0;
    for (int i = 0; i < sysdim_jacparams ; i++) {
        for (int k=0; k<nb_inputs[i] ; k++) {
            index_param[j] = i;
            if (k == 0)
                index_param_inv[i] = j;
            j++;
        }
    }
    // ******* end for variable inputs ********
    
    inputs = vector<AAF>(jacdim);
    if (sysdim_params > 0)
        params = vector<AAF>(sysdim_params);
    
    uncontrolled = 0;
    controlled = 0;
    is_uncontrolled = vector<bool>(sysdim+sysdim_params);
    variable = 0;
    is_variable = vector<bool>(sysdim+sysdim_params);
    // nb_inputs = vector<int>(jacdim);
    is_initialcondition = vector<bool>(sysdim+sysdim_params);
    for (int i=0 ; i<jacdim; i++) {
        is_uncontrolled[i] = false;  // controlled input or parameter
        is_variable[i] = false;     // variable input or parameter
       // nb_inputs[i] = 1; // > 1 if variable input
        is_initialcondition[i] = true; // by definition, initial conditions are controlled and constant
    }
    
    refined_mean_value = false;
    
    if (systype == 0) // ODE
    {
        t_begin = 0;
        if (syschoice == 1)  // running example
        {
            tau = 0.1;
            t_end = 2.;
            order = 3;
            
            inputs[0] = interval(0.9,1);
            params[0] = 1.0;
        }
        else if (syschoice == 2) // Brusselator
        {
            tau = 0.05;
            t_end = 10.;
            order = 4;
            
            inputs[0] = interval(0.9,1);
            inputs[1] = interval(0,0.1);
            
            params[0] = 1;
            params[1] = 1.5;
        }
        else if (syschoice == 3) // ballistic
        {
            tau = 0.1;
            t_end = 4.;
            order = 3;
            
            inputs[0] = interval(181.,185.); // velocity // interval(175.0,190.0); pour Eric
            // ix[1] = 3.14159/180*interval(2.5,3.5);  // angle   // interval(0,5) pour Eric
            //  ix[1] = mid(3.14159/180*interval(2.5,3.5)) + interval(-0.00872664, -0.00497644); // almost fault trajectories
            inputs[1] = interval(0.0436,0.0611); // 3.14159/180*interval(2.5,3.5); // mid(3.14159/180*interval(2.5,3.5)) + interval( -0.00497644,0.00872664); // complement = safe trajectories
            inputs[2] = interval(0.0,0.01);
            inputs[3] = interval(0.0,0.01);
        }
        else if (syschoice == 4) // ballistic linearise
        {
            tau = 0.1;
            t_end = 1.4;
            order = 3;
            
            inputs[0] = interval(181.,185.); // velocity // interval(175.0,190.0); pour Eric
            // ix[1] = 3.14159/180*interval(2.5,3.5);  // angle   // interval(0,5) pour Eric
            //  ix[1] = mid(3.14159/180*interval(2.5,3.5)) + interval(-0.00872664, -0.00497644); // almost fault trajectories
            inputs[1] =  interval(0.0436,0.0611); // 3.14159/180*interval(2.5,3.5); // mid(3.14159/180*interval(2.5,3.5)); // + interval( -0.00497644,0.00872664); // complement = safe trajectories
            inputs[2] = interval(0.0,0.01);
            inputs[3] = interval(0.0,0.25); // interval(0.0,0.01);
            inputs[4]= interval(11.,15.); // 14.... la masse (incontrollable)
            is_initialcondition[4] = false;
            is_uncontrolled[4] = true;
            is_variable[4] = true;
       //     nb_subdiv_init = 2;
            component_to_subdiv = 4;
        }
        else if (syschoice == 5) // self-driving car; sysdim = 2, jacdim = 2
        {
            tau = 0.05;
            t_end = 5.;
            order = 3;  // order of Taylor Models
            // initial condition
            inputs[0] = interval(-0.1,0.1);
            inputs[1] = interval(0,0.1);
            //  uncertain parameter 
            params[0] =  interval(1.9,2.1);  // Kp
            params[1] =  interval(2.9,3.1);    // Kd
        }
       /* else if (syschoice == 6) // self-driving car; sysdim = 2, jacdim = 4
        {  // DO NOT use for now sysdim != jacdim (temporary bug that will be fixed)
            tau = 0.05;
            t_end = 5.;
            order = 3;  // order of Taylor Models
            // uncertain parameter occurring in initial condition
            inputs[0] = interval(-0.1,0.1);
            inputs[1] = interval(0,0.1);
            inputs[2] =  interval(1.9,2.1);  // Kp
            inputs[3] =  interval(2.9,3.1);    // Kd
            is_initialcondition[0] = true;
            is_initialcondition[1] = true;
            is_uncontrolled[2] = true;
            is_uncontrolled[3] = true;
            uncontrolled = 2; // utile pour l'affichage
        } */
        else if (syschoice == 6) // self-driving car with piecewise constant parameters; sysdim = 4, jacdim = 4
        {
            tau = 0.02;
            t_end = 5.;
            order = 3;  // order of Taylor Models
            // uncertain parameter occurring in initial condition
            inputs[0] = interval(-0.1,0.1);
            inputs[1] = interval(0,0.1);
            inputs[2] =  interval(1.9,2.1);  // Kp
            inputs[3] =  interval(2.9,3.1);    // Kd
            //     inputs[4] = 0;
            is_initialcondition[2] = false;
            is_initialcondition[3] = false;
            is_uncontrolled[3] = true; // Kd uncontrolled
         //   is_variable[2] = true;  // piecewise constant
          //  is_uncontrolled[2] = true;  // Kp uncontrolled
          //   is_variable[3] = true; // piecewise constant
        }
        else if (syschoice == 7) // self-driving car with time varying parameters; sysdim = 4, jacdim = 4
        {
            tau = 0.02;
            t_end = 5.;
            order = 3;  // order of Taylor Models
            // uncertain parameter occurring in initial condition
            inputs[0] = interval(-0.1,0.1);
            inputs[1] = interval(0,0.1);
            inputs[2] =  interval(1.9,2.1);  // Kp
            inputs[3] =  interval(2.9,3.1);    // Kd
       //     inputs[4] = 0;
            is_initialcondition[2] = false;
            is_initialcondition[3] = false;
            is_uncontrolled[3] = true; // Kd uncontrolled
            is_variable[2] = true;  // attention, when changing from const to time-varying the differential system must also be modified in ode_def.h
          //  is_uncontrolled[2] = true;  // Kp uncontrolled
             is_variable[3] = true; // attention the differential system must also be modified in ode_def.h
        }
        else if (syschoice == 8)
        {
            tau = 0.01;
            t_end = 5.;
            order = 3;
            inputs[0] = interval(0.4,0.5);
        }
        else if (syschoice == 11)
        {
            tau = 1.;
            t_end = 2.;
            order = 4;
            inputs[0] = 1;
            inputs[1] = 0;
            inputs[2] = interval(0,0.1);
            inputs[3] = interval(0,0.1);
            is_initialcondition[2] = false;
            is_initialcondition[3] = false;
            //is_initialcondition[2] = false;
            is_variable[2] = true;
            is_variable[3] = true;
        }
        else if (syschoice == 12)
        {
            tau = 1.;
            t_end = 2.;
            order = 3;
            inputs[0] = 1;
            inputs[1] = 0;
            inputs[2] = interval(0,0.1);
            inputs[3] = interval(0,0.1);
            is_initialcondition[2] = false;
            is_initialcondition[3] = false;
            is_variable[2] = true;
            is_variable[3] = true;
           // is_variable[1] = true;
        }
        else if (syschoice == 13)  // Laub-Loomis Benchmark [Arch 2019]
        {
            tau = 0.1;
            t_end = 20.;
            order = 3;
            interval W = interval(-0.05,0.05);
            // to express that it is the same interval: use params[0] = interval(-0.1,0.1) and inputs[0] = 1.2 + params[0]; etc
            inputs[0] = 1.2 + W;
            inputs[1] = 1.05 + W;
            inputs[2] = 1.5 + W;
            inputs[3] = 2.4 + W;
            inputs[4] = 1. + W;
            inputs[5] = 0.1 + W;
            inputs[6] = 0.45 + W;
        }
        else if (syschoice == 14) // Van der Pol oscillator [Arch 2019]
        {
            tau = 0.01;
            t_end = 6.;
            order = 10;
            // for mu = 1
            inputs[0] = interval(1.25,1.55);
            inputs[1] = interval(2.35,2.45);
           // for mu = 2
            // inputs[0] = interval(1.55,1.85);
           // inputs[1] = interval(2.35,2.45);
        }
        else if (syschoice == 15) // Van der Pol oscillator [Arch 2018 and Sparse Polynomial zonotopes]
        {
            tau = 0.005;
            t_end = 3.15;
            order = 10;
            // for mu = 1
            inputs[0] = interval(1.23,1.57);
            inputs[1] = interval(2.34,2.46);
            // for mu = 2
            // inputs[0] = interval(1.55,1.85);
            // inputs[1] = interval(2.35,2.45);
        }
        else if(syschoice == 17) // quadrotor model [Arch 2019]
        {
            tau = 0.1;
            t_end = 5;
            order = 3;
            for (int j=0 ; j<sysdim; j++)
                inputs[j] = 0;
            // positions
            inputs[0] = interval(-0.4,0.4);
            inputs[1] = interval(-0.4,0.4);
            inputs[2] = interval(-0.4,0.4);
            // velocities
            inputs[3] = interval(-0.4,0.4);
            inputs[4] = interval(-0.4,0.4);
            inputs[5] = interval(-0.4,0.4);
        }
        else if (syschoice == 18) // crazyflie HSCC 2019 paper
        {   // do not forget to initialize the setpoints in the ode_def.h file...
            tau = 0.03;
            t_end = 2.;
            order = 3;
            
            for (int j=0 ; j<sysdim; j++)
                inputs[j] = 0;
            
            inputs[3] = interval(-0.00872,0.00872); // = interval(-0.5,0.5) * M_PI/180.0;  // p ?
            inputs[4] = interval(-0.00872,0.00872); //interval(-0.5,0.5) * M_PI/180.0;  // q ?
            inputs[12] = interval(-0.2,0.2); // * M_PI/180.0;  // z ?
            
            // roll yaw pitch (degree) inputs value (here we consider input as initial)
           // inputs[0] = interval(3.0 , 5.0) * M_PI/180.0;
           // inputs[1] = interval(3.0 , 5.0) * M_PI/180.0;
           // inputs[2] = 0.0 * M_PI/180.0;
            
            // p , q , r in rad/s -> the value here is an upper bound of the gyro noise of crazyflie
          //  inputs[3] = interval(-0.05,0.05);
          //  inputs[4] = interval(-0.05,0.05);
          //  inputs[5] = interval(-0.01,0.01);;
            
            // err_p , err_q , err_r
            inputs[6] = 0.0;
            inputs[7] = 0.0;
            inputs[8] = 0.0;
            
            // body speed u , v and w -> for embedded verif we instead use world speed
            inputs[9] = 0.0;
            inputs[10] = 0.0;
            inputs[11] = 0.0;
            
            // Z and err_Vz
          //  inputs[12] = interval(-0.1 , 0.1);
            inputs[13] = 0.0;
        }
        else if (syschoice == 19) {  // academic example, time-varying (piecewise constant) parameters
            tau = 1.0;
            t_end = 2;
            order = 4;
            inputs[0] = 0;
            inputs[1] = 0;
            inputs[2] = interval(0,1.);
            is_initialcondition[2] = false;
         //   is_variable[2] = true;
        //    nb_inputs[2] = 2; // piecewise constant input changes value every t_end/nb_inputs[i] seconds
            // solution at t=2 is 6 + u1 - u2 (u being the piecewise constant value of param_inputs[1] on each time interval)
        }
        else if (syschoice == 21) {  // academic example, time-varying (piecewise constant) parameters
            tau = 1.0;
            t_end = 2;
            order = 3;
            inputs[0] = 0;
            inputs[1] = 0;
            inputs[2] = interval(0,1.);
            for (int i=0; i<sysdim_jacparams; i++) {
                for (int j=1; j<nb_inputs[i]; j++)
                    inputs[sysdim+index_param_inv[i]+j] = inputs[sysdim+index_param_inv[i]].convert_int();
            }
            is_initialcondition[2] = false;
           // is_variable[2] = true;
          //  nb_inputs[2] = 1; // piecewise constant input changes value every t_end/nb_inputs[i] seconds
            // solution at t=2 is 6 + u1 - u2 (u being the piecewise constant value of param_inputs[1] on each time interval)
        }
        else if (syschoice == 22) {  // academic example, time-varying (piecewise constant) parameters
            tau = 1.;
            t_end = 2;
            order = 3;
            inputs[0] = 0;
            inputs[1] = 0;
            inputs[2] = interval(0,1.);
            for (int i=0; i<sysdim_jacparams; i++) {
                for (int j=1; j<nb_inputs[i]; j++)
                    inputs[sysdim+index_param_inv[i]+j] = inputs[sysdim+index_param_inv[i]].convert_int();
            }
            is_initialcondition[2] = false;
            // is_variable[2] = true;
            //  nb_inputs[2] = 1; // piecewise constant input changes value every t_end/nb_inputs[i] seconds
            // solution at t=2 is 6 + u1 - u2 (u being the piecewise constant value of param_inputs[1] on each time interval)
        }
    }
    if (systype == 1) // DDE
    {
        if (syschoice == 1)  // running example
        {
            d0 = 1; // delay in DDE
            // nb_subdiv = 50;
             nb_subdiv = 20;  // number of Taylor models on [0,d0] - defines the timestep here
            t_begin = -d0;  // starting time is -d0 (delay)
            // t_end = 15;
            t_end = 2.;  // final time
            // order = 3;
            order = 2;  // order of Taylor Models
            // uncertain parameter occurring in initial condition
            inputs[0] = interval(0.33,1.0);
            // nb_subdiv_init = 5;
        }
        else if (syschoice == 2)
        {
            d0 = 1; // delay in DDE
            nb_subdiv = 33;  // number of Taylor models on [0,d0]
            t_begin = -d0;  // starting time is -d0 (delay)
            t_end = 10.;
            order = 3;  // order of Taylor Models
            // uncertain parameter occurring in initial condition
            inputs[0] = interval(0.9,1.1);
            inputs[1] = interval(0.9,1.1);
        }
        else if (syschoice == 3)  // Xue 2017 (Ex 3)
        {
            d0 = 0.01; // delay in DDE
            nb_subdiv = 1;  // number of Taylor models on [0,d0]
            t_begin = 0.0;  // starting time is 0 (delay)
            t_end = 0.1;
            order = 2;  // order of Taylor Models
            // uncertain parameter occurring in initial condition
            inputs[0] = interval(1.1,1.2); // et non (1.1,1.3) comme indique dans le papier
            inputs[1] = interval(0.95,1.15);
            inputs[2] = interval(1.4,1.6);
            inputs[3] = interval(2.3,2.5);
            inputs[4] = interval(0.9,1.1);
            inputs[5] = interval(0.0,0.2);
            inputs[6] = interval(0.35,0.55);
        }
        else if (syschoice == 4)  // Szczelina 1  2014
        {
            d0 = 1.0;
            nb_subdiv = 10;  // number of Taylor models on [0,d0]
            t_begin = -d0;  // starting time is -d0 (delay)
            t_end = 2.;
            order = 2;  // order of Taylor Models
            // uncertain parameter occurring in initial condition
            inputs[0] = interval(0.9,1.1);
        }
        else if (syschoice == 5) // Szczelina 2  2014
        {
            d0 = 1.0;
            nb_subdiv = 10;  // number of Taylor models on [0,d0]
            t_begin = -d0;  // starting time is -d0 (delay)
            t_end = 2.;
            order = 3;  // order of Taylor Models
            // uncertain parameter occurring in initial condition
            inputs[0] = interval(0.9,1.1);
        }
        else if (syschoice == 6) // self-driving car; sysdim = 2, jacdim = 2
        {
            d0 = 0.2;
            nb_subdiv = 5;  // number of Taylor models on [0,d0]
            t_begin = -d0;  // starting time is -d0 (delay)
            t_end = 5.;
            order = 3;  // order of Taylor Models
            // uncertain parameter occurring in initial condition
            inputs[0] = interval(-0.1,0.1);
            inputs[1] = interval(0,0.1);
            params[0] =  interval(1.9,2.1); // 2;  // Kp
            params[1] =  interval(2.9,3.1);  // 3;   // Kd
        }
        else if (syschoice == 7) // self-driving car bt with coeff in interv; sysdim = 4, jacdim = 4
        {                         // this one can be forgotten ?
            d0 = 0.2;
            nb_subdiv = 5;  // number of Taylor models on [0,d0]
            t_begin = -d0;  // starting time is -d0 (delay)
            t_end = 5.;
            order = 3;  // order of Taylor Models
          //  uncontrolled = 2; // the last 2 parameters/inputs are uncontrolled (forall parameters)
            // uncertain parameter occurring in initial condition
            inputs[0] = interval(-0.1,0.1);
            inputs[1] = interval(0,0.1);
            inputs[2] = interval(1.9,2.1);  // Kp
            inputs[3] = interval(2.9,3.1);   // Kd
            is_initialcondition[2] = false;
            is_initialcondition[3] = false;
         //   is_uncontrolled[2] = true;
            is_uncontrolled[3] = true;
        }
        else if (syschoice == 8) // self-driving car bt with coeff in interv; sysdim = 2; jacdim = 4
        {
            d0 = 0.2;
            nb_subdiv = 5;  // number of Taylor models on [0,d0]
            t_begin = -d0;  // starting time is -d0 (delay)
            t_end = 5.;
            order = 3;  // order of Taylor Models
            // uncertain parameter occurring in initial condition
            inputs[0] = interval(-0.1,0.1);
            inputs[1] = interval(0,0.1);
            inputs[2] = interval(1.9,2.1);  // Kp
            inputs[3] = interval(2.9,3.1);   // Kd
            is_initialcondition[2] = false;
            is_initialcondition[3] = false;
        //    is_uncontrolled[2] = true;
            is_uncontrolled[3] = true;
        }
        else if (syschoice == 9) // Zou CAV 2015
        {
            d0 = 1.;
            nb_subdiv = 20;
            t_begin = 0;
            t_end = 2;
            order = 5;
            inputs[0] = interval(3.,6.);
        }
        else if (syschoice == 10)  // platoon of 5 vehicles
        {
            d0 = 0.3;
            t_begin = -d0;
            t_end = 10;
            nb_subdiv = 3;
            order = 3;
            inputs[0] = interval(-0.01,0.01);
            inputs[1] = interval(-1.2,-0.8);
            inputs[2] = interval(1.99,2.01);
            inputs[3] = interval(-2.2,-1.8);
            inputs[4] = interval(1.99,2.01);
            inputs[5] = interval(-3.2,-2.8);
            inputs[6] = interval(1.99,2.01);
            inputs[7] = interval(-4.2,-3.8);
            inputs[8] = interval(1.99,2.01);
        }
        else if (syschoice == 11)  // platoon of 10 vehicles
        {
            d0 = 0.3;
            t_begin = -d0;
            t_end = 10;
            nb_subdiv = 3;
            order = 3;
            inputs[0] = interval(-0.01,0.01);
            inputs[1] = interval(-1.2,-0.8);
            inputs[2] = interval(1.99,2.01);
            inputs[3] = interval(-2.2,-1.8);
            inputs[4] = interval(1.99,2.01);
            inputs[5] = interval(-3.2,-2.8);
            inputs[6] = interval(1.99,2.01);
            inputs[7] = interval(-4.2,-3.8);
            inputs[8] = interval(1.99,2.01);
            inputs[9] = interval(-5.2,-4.8);
            inputs[10] = interval(1.99,2.01);
            inputs[11] = interval(-6.2,-5.8);
            inputs[12] = interval(1.99,2.01);
            inputs[13] = interval(-7.2,-6.8);
            inputs[14] = interval(1.99,2.01);
            inputs[15] = interval(-8.2,-7.8);
            inputs[16] = interval(1.99,2.01);
            inputs[17] = interval(-9.2,-8.8);
            inputs[18] = interval(1.99,2.01);
        }
    }
    
    variables_to_display = vector<bool>(sysdim);
    for (int i=0; i< sysdim; i++)
        variables_to_display[i] = true;
    
    if (argc == 4) // called with configuration file: we overwrite the initialization of init_system
        read_parameters(argv[3], tau, t_end, d0, t_begin, order, nb_subdiv);
    
    
    if (systype == 0)
    {
        nb_points = ceil((t_end-t_begin)/tau)+2;
    }
    else // systype == 1
    {
        tau = d0/nb_subdiv;
        nb_points = (ceil((t_end-t_begin)/d0+2))*(nb_subdiv+1);
    }
    
    
    // common to EDO and DDE
    center_inputs = vector<AAF>(jacdim);
    eps = vector<interval>(jacdim);
    for (int i=0 ; i<jacdim ; i++)
    {
        if (is_uncontrolled[i])
            uncontrolled ++;
        if (!is_uncontrolled[i] && (i >= sysdim))
            controlled++;
        
        temp = inputs[i].convert_int();
        center_inputs[i] = mid(temp);
        eps[i] = temp-mid(temp);
    }
    
  //  cout << "controlled=" << controlled  << " uncontrolled=" << uncontrolled << endl;
    
    open_outputfiles(); // needs sysdim to be first defined but also controlled and uncontrolled...
    
    // for saving results
  //  cout << "(t_end-t_begin)*nb_subdiv/d0+1=" << ((t_end-t_begin)/d0+1)*(nb_subdiv+1) << endl;
    Xouter_print = vector<vector<vector<interval>>>(nb_subdiv_init+1,vector<vector<interval>>(nb_points, vector<interval>(sysdim)));
    Xouter_robust_print = vector<vector<vector<interval>>>(nb_subdiv_init+1,vector<vector<interval>>(nb_points, vector<interval>(sysdim)));
    Xouter_minimal_print = vector<vector<vector<interval>>>(nb_subdiv_init+1,vector<vector<interval>>(nb_points, vector<interval>(sysdim)));
    Xinner_print = vector<vector<vector<interval>>>(nb_subdiv_init+1,vector<vector<interval>>(nb_points, vector<interval>(sysdim)));
    Xinner_joint_print = vector<vector<vector<interval>>>(nb_subdiv_init+1,vector<vector<interval>>(nb_points, vector<interval>(sysdim)));
    Xinner_robust_print = vector<vector<vector<interval>>>(nb_subdiv_init+1,vector<vector<interval>>(nb_points, vector<interval>(sysdim)));
    Xinner_minimal_print = vector<vector<vector<interval>>>(nb_subdiv_init+1,vector<vector<interval>>(nb_points, vector<interval>(sysdim)));
    Xexact_print = vector<vector<vector<interval>>>(nb_subdiv_init+1,vector<vector<interval>>(nb_points, vector<interval>(sysdim)));
    t_print = vector<double>(nb_points);
        
}

void init_subdiv(int current_subdiv, vector<AAF> inputs_save, int param_to_subdivide)
{
 //   center_inputs = vector<AAF>(jacdim);
 //   eps = vector<interval>(jacdim);
    
    interval save = inputs_save[param_to_subdivide].convert_int();
    double delta = (save.sup()-save.inf())/nb_subdiv_init;
    if ((current_subdiv > 1) && (current_subdiv < nb_subdiv_init))
        inputs[param_to_subdivide] = interval(save.inf()+delta*(current_subdiv-1-recovering),save.inf()+delta*(current_subdiv+recovering));
    else if (current_subdiv == 1)
        inputs[param_to_subdivide] = interval(save.inf()+delta*(current_subdiv-1),save.inf()+delta*(current_subdiv+recovering));
    else if (current_subdiv == nb_subdiv_init)
        inputs[param_to_subdivide] = interval(save.inf()+delta*(current_subdiv-1-recovering),save.inf()+delta*(current_subdiv));
    cout << "inputs[param_to_subdivide] " << inputs[param_to_subdivide] << endl;
    
   
     interval   temp = inputs[param_to_subdivide].convert_int();
        center_inputs[param_to_subdivide] = mid(temp);
        eps[param_to_subdivide] = temp-mid(temp);
    
}


// initial condition on [t0,t0+d0] given as x = g(t)
vector <T<AAF>> Initfunc(const  T<AAF> &t, vector<AAF> &beta)
{
    vector<T<AAF>> res(sysdim);
    
    // by default
    for (int i=0 ; i<sysdim; i++)
    res[i] = beta[i];
    
    if (syschoice == 1)  // running example
        res[0] = (1+beta[0]*t)*(1+beta[0]*t);  // ix[0] = beta
    else if (syschoice == 2) //  example 5.15
    {
        res[0] = beta[0]*exp(t);
        res[1] = beta[1]*(1-exp(-1));
    }
    else if (syschoice == 4) // Szczelina_1 2014
    {
        res[0] = beta[0]*sin(M_PI*t/2.0);
    }
    else if (syschoice == 5) // Szczelina_2 2014
    {
        res[0] = beta[0]*sin(M_PI*t/2.0);
    }
    
    return res;
}



// initial condition on [-d0,0] given as x = g(t)
vector <T<F<AAF>>> Initfunc(const  T<F<AAF>> &t, vector<T<F<AAF>>> &beta)
{
    vector<T<F<AAF>>> res(sysdim);
    
    // by default
    for (int i=0 ; i<sysdim; i++)
    res[i] = beta[i];
    
    if (syschoice == 1)  // running example
        res[0] = (1+beta[0]*t)*(1+beta[0]*t);  // ix[0] = beta
    else if (syschoice == 2) //  example 5.15
    {
        res[0] = beta[0]*exp(t);
        res[1] = beta[1]*(1-exp(-1));
    }
    else if (syschoice == 4) // Szczelina_1 2014
    {
        res[0] = beta[0]*sin(M_PI*t/2.0);
    }
    else if (syschoice == 5) // Szczelina_2 2014
    {
        res[0] = beta[0]*sin(M_PI*t/2.0);
    }
    return res;
}



// analytical solution if any (for comparison purpose)
void AnalyticalSol(int current_iteration, vector<AAF> &beta, double d0)
{
  //  vector<interval> res(sysdim);
    vector<interval> Xouter_min(sysdim), Xouter_max(sysdim);
    
    double t = t_print[current_iteration];
    double beta_inf = beta[0].convert_int().inf();
    double beta_sup = beta[0].convert_int().sup();
    
    // running example
    //res[0] = (1+beta[0]*t)*(1+beta[0]*t);  // ix[0] = beta
    
    for (int i=0 ; i<sysdim ; i++)
        Xexact_print[0][current_iteration][i] = interval(1,-1); // no analytical solution : bot
    
    if (systype == 1)
    {
    if ((syschoice == 1) && beta_sup <= 1)  // running example
    {
        if (t <= 0)   // on [-d0,0], solution is defined by init function
        {
            Xouter_min[0] = ((1.+beta_inf*t)*(1.+beta_inf*t));
            Xouter_max[0] = ((1.+beta_sup*t)*(1.+beta_sup*t));
            Xexact_print[0][current_iteration][0] = hull(Xouter_min[0],Xouter_max[0]);
        }
        else if (t >= 0 && t <= d0)
        {
            Xouter_min[0] = exp((-1./(3.*beta_inf)*(pow(1.+(t-1.)*beta_inf,3)-pow(1.-beta_inf,3))));
            Xouter_max[0] = exp((-1./(3.*beta_sup)*(pow(1.+(t-1.)*beta_sup,3)-pow(1.-beta_sup,3))));
            Xexact_print[0][current_iteration][0] = hull(Xouter_min[0],Xouter_max[0]);
            
        }
        else if (t >= d0 && t <= 2*d0)
        {
            double aux, temp1, temp2;
            aux = exp((-1./(3.*beta_inf)*(pow(1.+(d0-1.)*beta_inf,3)-pow(1.-beta_inf,3))));
            temp1 = pow(1+(t-2)*beta_inf,3)/(3*beta_inf);
            temp2 = pow(1-beta_inf,3)/(3*beta_inf);
            Xouter_min[0] =  aux * exp(-exp(temp2)*pow(3*beta_inf,-2/3.0)*2.6789385347077476337*(gsl_sf_gamma_inc_P(1/3.0,temp1)-gsl_sf_gamma_inc_P(1/3.0,temp2)));
            aux = exp((-1./(3.*beta_sup)*(pow(1.+(d0-1.)*beta_inf,3)-pow(1.-beta_sup,3))));
            temp1 = pow(1+(t-2)*beta_sup,3)/(3*beta_sup);
            temp2 = pow(1-beta_sup,3)/(3*beta_sup);
            Xouter_max[0] =  aux * exp(-exp(temp2)*pow(3*beta_sup,-2/3.0)*2.6789385347077476337*(gsl_sf_gamma_inc_P(1/3.0,temp1)-gsl_sf_gamma_inc_P(1/3.0,temp2)));
            Xexact_print[0][current_iteration][0] = hull(Xouter_min[0],Xouter_max[0]);
            // res[0] = interval(min(inf(Xouter_min[0]),inf(Xouter_max[0])),max(sup(Xouter_min[0]),sup(Xouter_max[0])));
          //  cout << "beta_inf = " << beta_inf << "beta_sup=" << beta_sup << " res[0] = " << res[0] << endl;
        }
        else
        {
            Xexact_print[0][current_iteration][0] = interval(1,-1); // no analytical solution : bot
        }
        
    }
    else if (syschoice == 2) //  example 5.15
    {
        // example 5.15
        if (t <0)
        {
            Xexact_print[0][current_iteration][0] = beta[0].convert_int()*exp(t);
            Xexact_print[0][current_iteration][1] = beta[1].convert_int()*(1-exp(-1.));
        }
        else
        {
            Xexact_print[0][current_iteration][0] = beta[0].convert_int()*exp(t);
            Xexact_print[0][current_iteration][1] = beta[1].convert_int()*(exp(t)-exp(t-1.));
        }
    }
    }
    
    // iterative definition
    // mmmh idealementil faudrait calculer en AAF ici ???
    else if ((systype == 0) && ((syschoice == 19) || (syschoice == 20)|| (syschoice == 21)))
    {
        if (current_iteration == 0)
            Xexact_print[0][current_iteration][0] = inputs[0].convert_int();
        else {
            if (jacdim>sysdim+sysdim_jacparams) {
                double delta_t = t_print[current_iteration]-t_print[current_iteration-1];
                double delta_t_sq = t_print[current_iteration]*t_print[current_iteration]-t_print[current_iteration-1]*t_print[current_iteration-1];
                Xexact_print[0][current_iteration][0] = Xexact_print[0][current_iteration-1][0] + inputs[2].convert_int()*(2*delta_t-delta_t_sq) + 2*delta_t + delta_t_sq/2;
            }
            else {
                double delta_t = t_print[current_iteration]-t_print[0];
                double delta_t_sq = t_print[current_iteration]*t_print[current_iteration]-t_print[0]*t_print[0];
                Xexact_print[0][current_iteration][0] = inputs[0].convert_int() + inputs[2].convert_int()*(2*delta_t-delta_t_sq) + 2*delta_t + delta_t_sq/2;
            }
        }
    }
    else if ((systype == 0) && ((syschoice == 22)))
    {
        if (current_iteration == 0)
            Xexact_print[0][current_iteration][0] = inputs[0].convert_int();
        else {
            if (jacdim>sysdim+sysdim_jacparams) {
                double delta_t = t_print[current_iteration]-t_print[current_iteration-1];
                double delta_t_sq = t_print[current_iteration]*t_print[current_iteration]-t_print[current_iteration-1]*t_print[current_iteration-1];
                Xexact_print[0][current_iteration][0] = Xexact_print[0][current_iteration-1][0] + (inputs[2].convert_int()+inputs[2].convert_int()*inputs[2].convert_int())*(2*delta_t-delta_t_sq) + 2*delta_t + delta_t_sq/2;
            }
            else {
                double delta_t = t_print[current_iteration]-t_print[0];
                double delta_t_sq = t_print[current_iteration]*t_print[current_iteration]-t_print[0]*t_print[0];
                Xexact_print[0][current_iteration][0] = inputs[0].convert_int() + (inputs[2].convert_int()+inputs[2].convert_int()*inputs[2].convert_int())*(2*delta_t-delta_t_sq) + 2*delta_t + delta_t_sq/2;
            }
        }
    }
    
}

