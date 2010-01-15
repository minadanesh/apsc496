%Geometric Parameters
Length_1 = 5e-2;
Ratio_1to2 = 1.1;
Length_3 = 8e-2;
Theta_3_max = 45;
Length_4 = 7e-2;
Theta_4_max = 45;
Theta_1_max = 60;
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
plot3(0,0,0,'go')

%Nested Loops
for Theta_1 = Theta_1_max : -10*Step : Theta_1_min
   
    for Theta_2 = Theta_12_min : 7*Step : Theta_12_max
      
       for Theta_3 = Theta_3_min : 6*Step : Theta_3_max
         
          for Theta_4 = 0 : 5*Step : Theta_4_max
          
              %Tool Position
              x_position = Length_3 - (Length_4 * sin(Theta_4));
              x_position_neg = Length_3 + (Length_4 * sin(Theta_4));
              Theta_offset = Theta_2 + Theta_1 - (pi()/2);
              Theta_3_offset = Theta_3 + Theta_offset;
              y_position = Length_2 * cos(Theta_offset) - Length_1 * sin(Theta_1) + Length_4 * sin(Theta_3_offset);
              z_position = Length_1 * cos(Theta_1) + Length_2 * sin(Theta_offset) - Length_4 * cos(Theta_3_offset);
              
              plot3(x_position,y_position,z_position,'.')
              plot3(x_position_neg,y_position,z_position,'r.')
              
         end
          
      end
       
   end
    
    
end
