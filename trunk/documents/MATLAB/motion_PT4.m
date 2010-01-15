%Geometric Parameters
Length_1 = 7e-2;
Ratio_1to2 = 1.1;
Length_3 = 8e-2;
Theta_3_max = 45;
Length_4 = 7e-2;
Theta_4_max = 45;
Theta_1_max = 85;
Theta_12_min = 30;
Theta_12_max = 135;
Step = pi()/180;

%Calculated Lengths and Conversions to radians
Length_2 = Length_1 * Ratio_1to2;
Theta_3_max = Theta_3_max * pi() / 180;
Theta_4_max = Theta_4_max * pi() / 180;
Theta_1_max = Theta_1_max * pi() / 180;
Theta_1_min = -Theta_1_max ;
Theta_3_min = -Theta_3_max ;
Theta_12_min = Theta_12_min * pi() / 180 ;
Theta_12_max = Theta_12_max * pi() / 180 ;

hold on
subplot(2,2,1), plot3(0,0,0,'go'), hold on
subplot(2,2,2), plot3(0,0,0,'go'), hold on
subplot(2,2,3), plot3(0,0,0,'go'), hold on
subplot(2,2,4), plot3(0,0,0,'go'), hold on

%Nested Loops
for Theta_1 = Theta_1_max : -10*Step : Theta_1_min
   
    for Theta_12 = Theta_12_min : 7*Step : Theta_12_max
      
       %for Theta_3 = Theta_3_min : 6*Step : Theta_3_max
         %Theta_3 = 0;
          for Theta_4 = 0 : 5*Step : Theta_4_max
          
              %Tool Position
              x_position = Length_3 - (Length_4 * sin(Theta_4));
              x_position_neg = Length_3 + (Length_4 * sin(Theta_4));
              delta_z = Length_4 * (1 - cos(Theta_4));
              Length_2_star = (Length_2 ^2 + delta_z ^2) ^.5;
              Theta_star = tan(delta_z / Length_2) ;
              Theta_12_star = Theta_12 + Theta_star ;
              y_position = Length_2_star * cos(Theta_1 + Theta_12_star) - Length_1 * sin(Theta_1) ;
              z_position = Length_2_star * sin(Theta_1 + Theta_12_star) + Length_1 * cos(Theta_1) ;
              
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
