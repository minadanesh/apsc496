%Geometric Constants
Length_1 = 7e-2;
Length_2 = 1.1 * Length_1;
Length_3 = 7e-2;
Length_4 = 7e-2;
Length_5 = 7e-2;

%Angles in Degrees
Theta_23 = 80;
Theta_3 = 45;
Theta_35 = 0;

%Range of Motion in Degrees
Theta_r_min = -85;
Theta_r_max = 85;
Theta_12_min = 35;
Theta_12_max = 135;
Theta_5_min = -50;
Theta_5_max = 50;
Angle_step = 5;

%All angles in Radians
conv = pi() / 180;
Theta_23 = Theta_23 * conv;
Theta_3 = Theta_3 * conv;
Theta_35 = Theta_35 * conv;
Theta_r_min = Theta_r_min * conv;
Theta_r_max = Theta_r_max * conv;
Theta_12_min = Theta_12_min * conv;
Theta_12_max = Theta_12_max * conv;
Theta_5_min = Theta_5_min * conv;
Theta_5_max = Theta_5_max * conv;
Angle_step = Angle_step * conv;

hold on
subplot(2,1,1), plot3(0,0,0,'go'), hold on
subplot(2,1,2), plot3(0,0,0,'go'), hold on

%Nested Loops
for Theta_r = Theta_r_min : 2*Angle_step : Theta_r_max
   
    for Theta_12 = Theta_12_min : Angle_step : Theta_12_max
            
          for Theta_5 = 0 : Angle_step : Theta_5_max
          
              %Intermediate Calculations
              Length_5_eff = Length_5 * cos(Theta_5);
              radial_12 = (Length_1 ^2 + Length_2 ^2 - 2 * Length_1 * Length_2 * cos(Theta_12)) ^.5;
              radial_25 = (Length_3 ^2 + Length_5_eff ^2 - 2 * Length_3 * Length_5_eff * cos(Theta_35)) ^.5;
              Theta_25 = pi() - asin(Length_5_eff * sin(Theta_35 / radial_25)) + Theta_3;
              radial_end = (radial_12 ^2 + radial_25 ^2 - 2 * radial_12 * radial_25 * cos(Theta_25))^.5;
              Theta_end = asin(radial_25 * sin(Theta_25) / radial_end);
              
              %Tool Position
              
              x_position = Length_4 + Length_5 * sin(Theta_5);
              x_position_neg = Length_4 - Length_5 * sin(Theta_5);
              y_position = radial_end * sin(Theta_r + Theta_end);
              z_position = radial_end * cos(Theta_r + Theta_end);
                            
              
              hold all
              subplot(2,2,1), plot3(x_position,y_position,z_position,'.'), view(3), hold on
              subplot(2,2,1), plot3(x_position_neg,y_position,z_position,'r.'), view(3), hold on
              subplot(2,2,2), plot3(x_position,y_position,z_position,'.'), view(0,0), hold on
              subplot(2,2,2), plot3(x_position_neg,y_position,z_position,'r.'), view(0,0), hold on
              subplot(2,2,3), plot3(x_position,y_position,z_position,'.'), view(90,0), hold on
              subplot(2,2,3), plot3(x_position_neg,y_position,z_position,'r.'), view(90,0), hold on
              subplot(2,2,4), plot3(x_position,y_position,z_position,'.'), view(0,90), hold on
              subplot(2,2,4), plot3(x_position_neg,y_position,z_position,'r.'), view(0,90), hold on
              
              
         end
          
      %end
       
   end
    
    
end

