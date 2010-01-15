%Geometric Parameters
Theta_3_max = 45 ;
Radius_min = 6e-2 ;
Radius_max = 10e-2 ;
Length_3 = 8e-2 ;
Theta_1 = -45 ;
Length_4 = 7e-2 ;
Theta_4_max = 45 ;
Step = pi() / 180 ;

%Calculated Lengths and Conversions to radians
Theta_3_max = Theta_3_max * pi() / 180 ;
Theta_3_min = -Theta_3_max ;
Theta_1 = Theta_1 * pi() / 180 ;
Theta_4_max = Theta_4_max * pi() / 180 ;

hold on
subplot(2,2,1), plot3(0,0,0,'go'), hold on
subplot(2,2,2), plot3(0,0,0,'go'), hold on
subplot(2,2,3), plot3(0,0,0,'go'), hold on
subplot(2,2,4), plot3(0,0,0,'go'), hold on

%Nested Loops
%for Theta_3 = Theta_3_max : -10*Step : Theta_3_min
   
    for Radius = Radius_min : 3e-3 : Radius_max
      
       for Theta_3 = Theta_3_min : 6*Step : Theta_3_max
         
          for Theta_4 = 0 : 5*Step : Theta_4_max
          
              %Tool Position
              x_position = Length_3 - (Length_4 * sin(Theta_4));
              x_position_neg = Length_3 + (Length_4 * sin(Theta_4));
              y_position = Radius * sin(Theta_1) - Length_4 * cos(Theta_4) * sin(Theta_1 - Theta_3) ;
              z_position = Radius * cos(Theta_1) - Length_4 * cos(Theta_4) * cos(Theta_1 - Theta_3) ;
              
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
          
      end
       
   end
    
    
%end
