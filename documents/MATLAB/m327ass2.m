%% Adiabatic Flame Temp

%Number of Moles in per Mole of Fuel In
N_propane = .6 ;
N_butane = .4 ;
a = 5.6 ;
b = 3.4 ;
c = 4.4 ;
d = 21.056;

%Enthalpy of Formation - Reactants
h_form_propane = -103850 ;
h_form_butane = -126150 ;
h_form_o2 = 0 ;
h_form_N2 = 0 ;

%Enthalpy of Formation - Products
h_form_co2 = -393520 ;
h_form_h2o = -241820 ; %Assume h2o is a vapour

%Enthalpy at STP 
h_co2_stp = 9364 ;
h_h2o_stp = 9904 ;
h_N2_stp = 8669 ;
h_o2_stp = 8682 ;

%First Guess
Temp_1 = 2200 ; %input('What Temperature would you like to try first? ');
h_co2 = 112939; %input('What is the enthalpy of CO2 at this temperature? ');
h_h2o = 92940; % input('What is the enthalpy of h2o at this temperature? ');
h_N2 = 72040; %input('What is the enthalpy of N2 at this temperature? ');

%Total Energy
Energy_in = (N_propane * h_form_propane) + (N_butane * h_form_butane) ;
E_co2 = (b * (h_form_co2 + h_co2 - h_co2_stp));
E_h2o = (c * (h_form_h2o + h_h2o - h_h2o_stp)) ;
E_N2 = (d * (h_N2 - h_N2_stp));
Energy_out =  E_co2 + E_h2o + E_N2;
delta_E = Energy_in - Energy_out ;

%High of Low?
if delta_E > 0
    Temp_2 = 2400; %input('Enter a higher temperature: ') ;
    h_co2 = 125152; %input('What is the enthalpy of CO2 at this temperature? ');
    h_h2o = 103508; %input('What is the enthalpy of h2o at this temperature? ');
    h_N2 = 79320; %input('What is the enthalpy of N2 at this temperature? ');
    
else
    Temp_2 = input('Enter a lower temperature: ') ;
    h_co2 = input('What is the enthalpy of CO2 at this temperature? ');
    h_h2o = input('What is the enthalpy of h2o at this temperature? ');
    h_N2 = input('What is the enthalpy of N2 at this temperature? ');

end

%Total Energy
Energy_in = (N_propane * h_form_propane) + (N_butane * h_form_butane) ;
E_co2 = (b * (h_form_co2 + h_co2 - h_co2_stp));
E_h2o = (c * (h_form_h2o + h_h2o - h_h2o_stp)) ;
E_N2 = (d * (h_N2 - h_N2_stp));
Energy_out_2 =  E_co2 + E_h2o + E_N2;
delta_E_2 = Energy_in - Energy_out_2;

%Interpolate Between Temperatures
n = delta_E / (Energy_out_2 - Energy_out);
Adiabatic_Flame_Temp = Temp_1 + n * (Temp_2 - Temp_1);

disp('a. The adiabatic flame temperature is:')
disp(round(Adiabatic_Flame_Temp))
disp('')
%% Excess Air For Flame Temperature of 1490K

%New Properties at 1490K
F_Temp = 1490;
T_1 = 1480;
T_2 = 1500;
h_N2_1 = 46377 ;
h_N2_2 = 47073 ;
h_o2_1 = 48561 ;
h_o2_2 = 49292 ;
h_co2_1 = 66911 ;
h_co2_2 = 71078 ;
h_h2o_1 = 57062 ;
h_h2o_2 = 57999 ;

h_N2_fT = h_N2_1 + ((F_Temp - T_1)/(T_2 - T_1)) * (h_N2_2 - h_N2_1);
h_o2_fT = h_o2_1 + ((F_Temp - T_1)/(T_2 - T_1)) * (h_o2_2 - h_o2_1);
h_co2_fT = h_co2_1 + ((F_Temp - T_1)/(T_2 - T_1)) * (h_co2_2 - h_co2_1);
h_h2o_fT = h_h2o_1 + ((F_Temp - T_1)/(T_2 - T_1)) * (h_h2o_2 - h_h2o_1);

%Excess Air Required
Nh_p = N_propane * h_form_propane ;
Nh_b = N_butane * h_form_butane ;
Nh_co2 = b * (h_form_co2 + h_co2_fT - h_co2_stp);
Nh_h2o = c * (h_form_h2o + h_h2o_fT - h_h2o_stp);
Nh_N2 = d * (h_N2_fT - h_N2_stp) ;
delta_h_o2 = (h_o2_fT - h_o2_stp);
delta_h_N2 = 3.76*(h_N2_fT - h_N2_stp);
alpha = (Nh_p + Nh_b - Nh_co2 - Nh_h2o - Nh_N2) / (delta_h_o2 + delta_h_N2);
lambda = (alpha + a) / a ;

disp('b. The percentage of excess air required for a flame temperature of 1490K is:')
disp((lambda-1)*100)
disp('')
%% Irreversibility

%Constants
R_molar = 8.314;
T_environment = 298.15 ;

%Entropy at Standard Temperature
s_propane = 269.91 + R_molar * log(N_propane);
s_butane = 310.12 + R_molar * log(N_butane);
s_N2_sT = 241.301 + .5 * (241.768 - 241.301) ;
s_o2_sT = 257.474 + .5 * (257.965 - 257.474) ;
s_o2_sT_r = 205.033 ;
s_N2_sT_r = 191.502 ;
s_co2_sT = 291.333 + .5 * (292.114 - 291.333) ;
s_h2o_sT = 249.820 + .5 * (250.450 - 249.820) ;

%Molar Fractions of Reactants
N_tot_reactants = (a + alpha) + 3.76*(a + alpha);
y_propane = N_propane  ;
y_butane = N_butane  ;
y_o2_r = (a + alpha) / N_tot_reactants ;
y_N2_r = 3.76 * (a + alpha) / N_tot_reactants ;

%Molar Fractions of Products
N_tot_products = b + c + d + alpha * 4.76 ;
y_co2 = b / N_tot_products ;
y_h2o = c / N_tot_products ;
y_N2_p = (d + 3.76 * alpha) / N_tot_products ;
y_o2_p = alpha / N_tot_products ;

%Entropy of Reactants
s_N2_r = s_N2_sT_r - (R_molar * log(.79)) ;
s_o2_r = s_o2_sT_r - (R_molar * log(.21)) ;

%Entropy of Products
s_co2 = s_co2_sT - (R_molar * log(y_co2)) ;
s_h2o = s_h2o_sT - (R_molar * log(y_h2o)) ;
s_N2_p = s_N2_sT - (R_molar * log(y_N2_p)) ;
s_o2_p = s_o2_sT - (R_molar * log(y_o2_p)) ;

%Sum of N * s for Reactants and Products
Ns_p = N_propane * s_propane ;
Ns_b = N_butane * s_butane ;
Ns_o2 = (a + alpha) * s_o2_r ;
Ns_Reactants = Ns_p + Ns_b + Ns_o2 + 3.76 * (a + alpha) * s_N2_r ;
Ns_co2 = b * s_co2 ;
Ns_h2o = c * s_h2o ;
Ns_N2 = (d + 3.76*alpha) * s_N2_p ;
Ns_Products = Ns_co2 + Ns_h2o +Ns_N2 + alpha * s_o2_p ;

%Irreversibility
Irreverse = T_environment * (Ns_Products - Ns_Reactants);

disp('c. The irreversibility of the combustion is:')
disp(round(Irreverse))
disp('')


%% Chemical Exergy

%Gibbs Energy of Reactants
g_propane = h_form_propane - T_environment * s_propane ;
g_butane = h_form_butane - T_environment * s_butane ;
g_o2_r = h_form_o2 - T_environment * s_o2_r;
g_N2_r = h_form_N2 - T_environment * s_N2_r;

%Gibbs Energy of Products
g_co2 = h_form_co2 + h_co2_fT - h_co2_stp - T_environment * s_co2;
g_h2o = h_form_h2o + h_h2o_fT - h_h2o_stp - T_environment * s_h2o;
g_N2_p = h_form_N2 + h_h2o_fT - h_N2_stp - T_environment * s_N2_p;
g_o2_p = h_form_o2 + h_o2_fT - h_o2_stp - T_environment * s_o2_p;

%Maximum Reverisble Work
Ng_p = N_propane * g_propane;
Ng_b = N_butane * g_butane ;
Ng_Reactants = Ng_p + Ng_b + (.21) * g_o2_r + .79 * g_N2_r;
Ng_co2 = b * g_co2;
Ng_h2o = c * g_h2o;
Ng_N2 = (d + 3.76 * alpha) * g_N2_p ;
Ng_Products = Ng_co2 + Ng_h2o + Ng_N2 + alpha * g_o2_p;

Work_Rev = (Ng_Reactants - Ng_Products);


%Exergy of Hot gas
Exergy_hg = Work_Rev - Irreverse ;

disp('The Exergy of the hot gas is:')
disp(round(Exergy_hg))