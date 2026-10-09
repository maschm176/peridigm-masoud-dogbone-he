#!/usr/bin/env python

import sys
from lxml import etree
# import os.path

# geometry info
bar_length = 4.0 #(mm)
bar_thickness = 1.0 #(mm)
extensometer_length = 3.52 #(mm)

grid_size = 0.15 #(mm)
strain_rate = 350
disp_rate = strain_rate * bar_length / 2.0 #prescribed displacement on both side of the bar
final_strain = 18.0 #(%) -- calibration_strain_data_HE_0.txt only goes to ~15.4%
final_time = final_strain/100.0 / strain_rate 
time_step = 2e-8
desired_number_of_written_output1 = 1
desired_number_of_written_output2 = 300
output_frequency1 = int(final_time/time_step / desired_number_of_written_output1)
output_frequency2 = int(final_time/time_step / desired_number_of_written_output2)

class calibration_test():
    """
       Class to create an Input Deck for the calibration tests
    """

    def __init__(self, element_size, barThickness, extensometerLength, dispRate, finalTime, timeStep, outputFrequency1, outputFrequency2):

        self.grid_size = element_size
        self.bar_thickness = barThickness
        self.extensometer_length = extensometerLength
        # For a tetrahedral mesh
        self.horizon_size = 0.42
        self.top_disp = str(dispRate) + '*t'
        self.bottom_disp = '-' + str(dispRate) + '*t'
        self.final_time = finalTime
        self.time_step = timeStep
        self.output_frequency1 = outputFrequency1
        self.output_frequency2 = outputFrequency2

    def __add_Discretization(self,root):
        DSCRTZ = etree.SubElement(root, "ParameterList", name="Discretization")
        etree.SubElement(DSCRTZ, "Parameter", name="Type", type="string", value="Exodus")
        etree.SubElement(DSCRTZ, "Parameter", name="Input Mesh File", type="string", value="mesh_tensile/mesh.g")

    def __add_Materials(self,root):
        MTRL = etree.SubElement(root, "ParameterList", name="Materials")
        MTRL1 = etree.SubElement(MTRL, "ParameterList", name="My Material")
        etree.SubElement(MTRL1, "Parameter", name="Material Model", type="string", value="Isotropic Hardening Hypoelastic Correspondence")
        etree.SubElement(MTRL1, "Parameter", name="Density", type="double", value="7.8e-6")
        etree.SubElement(MTRL1, "Parameter", name="Young's Modulus", type="double", value="180.0e6")
        etree.SubElement(MTRL1, "Parameter", name="Poisson's Ratio", type="double", value="0.3")
        etree.SubElement(MTRL1, "Parameter", name="Yield Stress", type="double", value=str(self.par1))
        etree.SubElement(MTRL1, "Parameter", name="Hardening Characteristic Strain", type="double", value=str(self.par2))
        etree.SubElement(MTRL1, "Parameter", name="Hardening Exponent", type="double", value=str(self.par3))
        etree.SubElement(MTRL1, "Parameter", name="Actual Horizon", type="double", value=str(self.horizon_size))
        # DMG = etree.SubElement(MTRL1, "ParameterList", name="Damage")
        # etree.SubElement(DMG, "Parameter", name="Damage Model", type="string", value="Johnson-Cook")
        # etree.SubElement(DMG, "Parameter", name="Damage Parameter 1", type="double", value=str(self.par5))
        # etree.SubElement(DMG, "Parameter", name="Damage Parameter 2", type="double", value=str(self.par6))
        # etree.SubElement(DMG, "Parameter", name="Damage Parameter 3", type="double", value=str(self.par7))

    def __add_Blocks(self,root):
        BLK = etree.SubElement(root, "ParameterList", name="Blocks")
        BLK1 = etree.SubElement(BLK, "ParameterList", name="My First Block")
        etree.SubElement(BLK1, "Parameter", name="Block Names", type="string", value="block_2")
        etree.SubElement(BLK1, "Parameter", name="Material", type="string", value="My Material")
        # etree.SubElement(BLK1, "Parameter", name="Horizon", type="double", value="0.602")
        etree.SubElement(BLK1, "Parameter", name="Horizon", type="double", value=str(self.horizon_size*1.3))
        BLK2 = etree.SubElement(BLK, "ParameterList", name="My Other Blocks")
        etree.SubElement(BLK2, "Parameter", name="Block Names", type="string", value="default")
        etree.SubElement(BLK2, "Parameter", name="Material", type="string", value="My Material")
        # etree.SubElement(BLK2, "Parameter", name="Horizon", type="double", value="0.602")
        etree.SubElement(BLK2, "Parameter", name="Horizon", type="double", value=str(self.horizon_size*1.3))

    def __add_BC(self,root):
        BC = etree.SubElement(root, "ParameterList", name="Boundary Conditions")
        BC2 = etree.SubElement(BC, "ParameterList", name="Prescribed Displacement Bottom X")
        etree.SubElement(BC2, "Parameter", name="Type", type="string", value="Prescribed Displacement")
        etree.SubElement(BC2, "Parameter", name="Node Set", type="string", value="nodelist_4")
        etree.SubElement(BC2, "Parameter", name="Coordinate", type="string", value="x")
        etree.SubElement(BC2, "Parameter", name="Value", type="string", value="0.0")
        BC3 = etree.SubElement(BC, "ParameterList", name="Prescribed Displacement Bottom Y")
        etree.SubElement(BC3, "Parameter", name="Type", type="string", value="Prescribed Displacement")
        etree.SubElement(BC3, "Parameter", name="Node Set", type="string", value="nodelist_1")
        etree.SubElement(BC3, "Parameter", name="Coordinate", type="string", value="y")
        # etree.SubElement(BC3, "Parameter", name="Value", type="string", value="0.001*y*t/1e-4")
        etree.SubElement(BC3, "Parameter", name="Value", type="string", value=self.bottom_disp)
        BC4 = etree.SubElement(BC, "ParameterList", name="Prescribed Displacement Bottom Z")
        etree.SubElement(BC4, "Parameter", name="Type", type="string", value="Prescribed Displacement")
        etree.SubElement(BC4, "Parameter", name="Node Set", type="string", value="nodelist_3")
        etree.SubElement(BC4, "Parameter", name="Coordinate", type="string", value="z")
        etree.SubElement(BC4, "Parameter", name="Value", type="string", value="0.0")
        BC5 = etree.SubElement(BC, "ParameterList", name="Prescribed Displacement Top X")
        etree.SubElement(BC5, "Parameter", name="Type", type="string", value="Prescribed Displacement")
        etree.SubElement(BC5, "Parameter", name="Node Set", type="string", value="nodelist_6")
        etree.SubElement(BC5, "Parameter", name="Coordinate", type="string", value="x")
        etree.SubElement(BC5, "Parameter", name="Value", type="string", value="0.0")
        BC6 = etree.SubElement(BC, "ParameterList", name="Prescribed Displacement Top Y")
        etree.SubElement(BC6, "Parameter", name="Type", type="string", value="Prescribed Displacement")
        etree.SubElement(BC6, "Parameter", name="Node Set", type="string", value="nodelist_2")
        etree.SubElement(BC6, "Parameter", name="Coordinate", type="string", value="y")
        # etree.SubElement(BC6, "Parameter", name="Value", type="string", value="0.001*y*t/1e-4")
        etree.SubElement(BC6, "Parameter", name="Value", type="string", value=self.top_disp)
        BC7 = etree.SubElement(BC, "ParameterList", name="Prescribed Displacement Top Z")
        etree.SubElement(BC7, "Parameter", name="Type", type="string", value="Prescribed Displacement")
        etree.SubElement(BC7, "Parameter", name="Node Set", type="string", value="nodelist_5")
        etree.SubElement(BC7, "Parameter", name="Coordinate", type="string", value="z")
        etree.SubElement(BC7, "Parameter", name="Value", type="string", value="0.0")

    def __add_solver(self,root):
        SLVR = etree.SubElement(root, "ParameterList", name="Solver")
        etree.SubElement(SLVR, "Parameter", name="Verbose", type="bool", value="true")
        etree.SubElement(SLVR, "Parameter", name="Initial Time", type="double", value="0.0")
        # etree.SubElement(SLVR, "Parameter", name="Final Time", type="double", value="200.0e-4")
        etree.SubElement(SLVR, "Parameter", name="Final Time", type="double", value=str(self.final_time))
        SLVR2 = etree.SubElement(SLVR, "ParameterList", name="Verlet")
        # etree.SubElement(SLVR2, "Parameter", name="Fixed dt", type="double", value="0.5e-8")
        etree.SubElement(SLVR2, "Parameter", name="Fixed dt", type="double", value=str(self.time_step))
 	
    def __add_compute_classes(self,root):
        CMPT = etree.SubElement(root, "ParameterList", name="Compute Class Parameters")
        # CMPT1 = etree.SubElement(CMPT, "ParameterList", name="Top Reaction Force")
        # etree.SubElement(CMPT1, "Parameter", name="Compute Class", type="string", value="Block_Data")
        # etree.SubElement(CMPT1, "Parameter", name="Calculation Type", type="string", value="Sum")
        # etree.SubElement(CMPT1, "Parameter", name="Block", type="string", value="block_3")
        # etree.SubElement(CMPT1, "Parameter", name="Variable", type="string", value="Force")
        # etree.SubElement(CMPT1, "Parameter", name="Output Label", type="string", value="Top_Reaction_Force")
        CMPT2 = etree.SubElement(CMPT, "ParameterList", name="Bottom Reaction Force")
        etree.SubElement(CMPT2, "Parameter", name="Compute Class", type="string", value="Block_Data")
        etree.SubElement(CMPT2, "Parameter", name="Calculation Type", type="string", value="Sum")
        etree.SubElement(CMPT2, "Parameter", name="Block", type="string", value="block_1")
        etree.SubElement(CMPT2, "Parameter", name="Variable", type="string", value="Force")
        etree.SubElement(CMPT2, "Parameter", name="Output Label", type="string", value="Bottom_Reaction_Force")

        CMPT3 = etree.SubElement(CMPT, "ParameterList", name="Coordinates of Top PD Node Nearest 1")
        etree.SubElement(CMPT3, "Parameter", name="Compute Class", type="string", value="Nearest_Point_Data")
        etree.SubElement(CMPT3, "Parameter", name="X", type="double", value="0.0")
        etree.SubElement(CMPT3, "Parameter", name="Y", type="double", value=str(self.extensometer_length/2.0))
        etree.SubElement(CMPT3, "Parameter", name="Z", type="double", value=str(self.bar_thickness/2.0))
        etree.SubElement(CMPT3, "Parameter", name="Variable", type="string", value="Coordinates")
        etree.SubElement(CMPT3, "Parameter", name="Output Label", type="string", value="Coordinates_Top_Particle_1")
        CMPT4 = etree.SubElement(CMPT, "ParameterList", name="Coordinates of Top PD Node Nearest 2")
        etree.SubElement(CMPT4, "Parameter", name="Compute Class", type="string", value="Nearest_Point_Data")
        etree.SubElement(CMPT4, "Parameter", name="X", type="double", value=str(self.grid_size))
        etree.SubElement(CMPT4, "Parameter", name="Y", type="double", value=str(self.extensometer_length/2.0))
        etree.SubElement(CMPT4, "Parameter", name="Z", type="double", value=str(self.bar_thickness/2.0))
        etree.SubElement(CMPT4, "Parameter", name="Variable", type="string", value="Coordinates")
        etree.SubElement(CMPT4, "Parameter", name="Output Label", type="string", value="Coordinates_Top_Particle_2")
        CMPT5 = etree.SubElement(CMPT, "ParameterList", name="Coordinates of Top PD Node Nearest 3")
        etree.SubElement(CMPT5, "Parameter", name="Compute Class", type="string", value="Nearest_Point_Data")
        etree.SubElement(CMPT5, "Parameter", name="X", type="double", value="0.0")
        etree.SubElement(CMPT5, "Parameter", name="Y", type="double", value=str(self.extensometer_length/2.0+self.grid_size))
        etree.SubElement(CMPT5, "Parameter", name="Z", type="double", value=str(self.bar_thickness/2.0))
        etree.SubElement(CMPT5, "Parameter", name="Variable", type="string", value="Coordinates")
        etree.SubElement(CMPT5, "Parameter", name="Output Label", type="string", value="Coordinates_Top_Particle_3")
        CMPT6 = etree.SubElement(CMPT, "ParameterList", name="Coordinates of Top PD Node Nearest 4")
        etree.SubElement(CMPT6, "Parameter", name="Compute Class", type="string", value="Nearest_Point_Data")
        etree.SubElement(CMPT6, "Parameter", name="X", type="double", value=str(-self.grid_size))
        etree.SubElement(CMPT6, "Parameter", name="Y", type="double", value=str(self.extensometer_length/2.0))
        etree.SubElement(CMPT6, "Parameter", name="Z", type="double", value=str(self.bar_thickness/2.0))
        etree.SubElement(CMPT6, "Parameter", name="Variable", type="string", value="Coordinates")
        etree.SubElement(CMPT6, "Parameter", name="Output Label", type="string", value="Coordinates_Top_Particle_4")
        CMPT7 = etree.SubElement(CMPT, "ParameterList", name="Coordinates of Top PD Node Nearest 5")
        etree.SubElement(CMPT7, "Parameter", name="Compute Class", type="string", value="Nearest_Point_Data")
        etree.SubElement(CMPT7, "Parameter", name="X", type="double", value="0.0")
        etree.SubElement(CMPT7, "Parameter", name="Y", type="double", value=str(self.extensometer_length/2.0-self.grid_size))
        etree.SubElement(CMPT7, "Parameter", name="Z", type="double", value=str(self.bar_thickness/2.0))
        etree.SubElement(CMPT7, "Parameter", name="Variable", type="string", value="Coordinates")
        etree.SubElement(CMPT7, "Parameter", name="Output Label", type="string", value="Coordinates_Top_Particle_5")

        CMPT8 = etree.SubElement(CMPT, "ParameterList", name="Coordinates of Bottom PD Node Nearest 1")
        etree.SubElement(CMPT8, "Parameter", name="Compute Class", type="string", value="Nearest_Point_Data")
        etree.SubElement(CMPT8, "Parameter", name="X", type="double", value="0.0")
        etree.SubElement(CMPT8, "Parameter", name="Y", type="double", value=str(-self.extensometer_length/2.0))
        etree.SubElement(CMPT8, "Parameter", name="Z", type="double", value=str(self.bar_thickness/2.0))
        etree.SubElement(CMPT8, "Parameter", name="Variable", type="string", value="Coordinates")
        etree.SubElement(CMPT8, "Parameter", name="Output Label", type="string", value="Coordinates_Bottom_Particle_1")
        CMPT9 = etree.SubElement(CMPT, "ParameterList", name="Coordinates of Bottom PD Node Nearest 2")
        etree.SubElement(CMPT9, "Parameter", name="Compute Class", type="string", value="Nearest_Point_Data")
        etree.SubElement(CMPT9, "Parameter", name="X", type="double", value=str(self.grid_size))
        etree.SubElement(CMPT9, "Parameter", name="Y", type="double", value=str(-self.extensometer_length/2.0))
        etree.SubElement(CMPT9, "Parameter", name="Z", type="double", value=str(self.bar_thickness/2.0))
        etree.SubElement(CMPT9, "Parameter", name="Variable", type="string", value="Coordinates")
        etree.SubElement(CMPT9, "Parameter", name="Output Label", type="string", value="Coordinates_Bottom_Particle_2")
        CMPT10 = etree.SubElement(CMPT, "ParameterList", name="Coordinates of Bottom PD Node Nearest 3")
        etree.SubElement(CMPT10, "Parameter", name="Compute Class", type="string", value="Nearest_Point_Data")
        etree.SubElement(CMPT10, "Parameter", name="X", type="double", value="0.0")
        etree.SubElement(CMPT10, "Parameter", name="Y", type="double", value=str(-self.extensometer_length/2.0+self.grid_size))
        etree.SubElement(CMPT10, "Parameter", name="Z", type="double", value=str(self.bar_thickness/2.0))
        etree.SubElement(CMPT10, "Parameter", name="Variable", type="string", value="Coordinates")
        etree.SubElement(CMPT10, "Parameter", name="Output Label", type="string", value="Coordinates_Bottom_Particle_3")
        CMPT11 = etree.SubElement(CMPT, "ParameterList", name="Coordinates of Bottom PD Node Nearest 4")
        etree.SubElement(CMPT11, "Parameter", name="Compute Class", type="string", value="Nearest_Point_Data")
        etree.SubElement(CMPT11, "Parameter", name="X", type="double", value=str(-self.grid_size))
        etree.SubElement(CMPT11, "Parameter", name="Y", type="double", value=str(-self.extensometer_length/2.0))
        etree.SubElement(CMPT11, "Parameter", name="Z", type="double", value=str(self.bar_thickness/2.0))
        etree.SubElement(CMPT11, "Parameter", name="Variable", type="string", value="Coordinates")
        etree.SubElement(CMPT11, "Parameter", name="Output Label", type="string", value="Coordinates_Bottom_Particle_4")
        CMPT12 = etree.SubElement(CMPT, "ParameterList", name="Coordinates of Bottom PD Node Nearest 5")
        etree.SubElement(CMPT12, "Parameter", name="Compute Class", type="string", value="Nearest_Point_Data")
        etree.SubElement(CMPT12, "Parameter", name="X", type="double", value="0.0")
        etree.SubElement(CMPT12, "Parameter", name="Y", type="double", value=str(-self.extensometer_length/2.0-self.grid_size))
        etree.SubElement(CMPT12, "Parameter", name="Z", type="double", value=str(self.bar_thickness/2.0))
        etree.SubElement(CMPT12, "Parameter", name="Variable", type="string", value="Coordinates")
        etree.SubElement(CMPT12, "Parameter", name="Output Label", type="string", value="Coordinates_Bottom_Particle_5")

    def __add_output(self,root):
        OUT1 = etree.SubElement(root, "ParameterList", name="Output")
        etree.SubElement(OUT1, "Parameter", name="Output File Type", type="string", value="ExodusII")
        etree.SubElement(OUT1, "Parameter", name="Output Format", type="string", value="BINARY")
        etree.SubElement(OUT1, "Parameter", name="Output Filename", type="string", value="Output/ExodusOutputLongitudinal")
        # etree.SubElement(OUT1, "Parameter", name="Output Frequency", type="int", value="6000")
        etree.SubElement(OUT1, "Parameter", name="Output Frequency", type="int", value=str(self.output_frequency1))
        etree.SubElement(OUT1, "Parameter", name="Parallel Write", type="bool", value="true")
        OUT11 = etree.SubElement(OUT1, "ParameterList", name="Output Variables")
        # etree.SubElement(OUT2, "Parameter", name="Volume", type="bool", value="true")
        # etree.SubElement(OUT11, "Parameter", name="Displacement", type="bool", value="true")
        # etree.SubElement(OUT2, "Parameter", name="Velocity", type="bool", value="true")
        etree.SubElement(OUT11, "Parameter", name="Force", type="bool", value="true")
        # # etree.SubElement(OUT2, "Parameter", name="Element_Id", type="bool", value="true")
        # # etree.SubElement(OUT2, "Parameter", name="Proc_Num", type="bool", value="true")
        # etree.SubElement(OUT2, "Parameter", name="Force_Density", type="bool", value="true")
        # etree.SubElement(OUT2, "Parameter", name="Right_Cauchy_Green_Deformation", type="bool", value="true")
        # etree.SubElement(OUT2, "Parameter", name="Second_Piola_Stress", type="bool", value="true")
        # etree.SubElement(OUT2, "Parameter", name="Plastic_RCG_Deformation_Inv", type="bool", value="true")
        # etree.SubElement(OUT2, "Parameter", name="Damage", type="bool", value="true")
        # etree.SubElement(OUT2, "Parameter", name="Top_Reaction_Force", type="bool", value="true")

        OUT2 = etree.SubElement(root, "ParameterList", name="Output 2")
        etree.SubElement(OUT2, "Parameter", name="Output File Type", type="string", value="ExodusII")
        etree.SubElement(OUT2, "Parameter", name="Output Filename", type="string", value="Output/ExodusOutputLongitudinal")
        etree.SubElement(OUT2, "Parameter", name="Output Frequency", type="int", value=str(self.output_frequency2))
        OUT21 = etree.SubElement(OUT2, "ParameterList", name="Output Variables")
        etree.SubElement(OUT21, "Parameter", name="Bottom_Reaction_Force", type="bool", value="true")
        etree.SubElement(OUT21, "Parameter", name="Coordinates_Top_Particle_1", type="bool", value="true")
        etree.SubElement(OUT21, "Parameter", name="Coordinates_Top_Particle_2", type="bool", value="true")
        etree.SubElement(OUT21, "Parameter", name="Coordinates_Top_Particle_3", type="bool", value="true")
        etree.SubElement(OUT21, "Parameter", name="Coordinates_Top_Particle_4", type="bool", value="true")
        etree.SubElement(OUT21, "Parameter", name="Coordinates_Top_Particle_5", type="bool", value="true")
        etree.SubElement(OUT21, "Parameter", name="Coordinates_Bottom_Particle_1", type="bool", value="true")
        etree.SubElement(OUT21, "Parameter", name="Coordinates_Bottom_Particle_2", type="bool", value="true")
        etree.SubElement(OUT21, "Parameter", name="Coordinates_Bottom_Particle_3", type="bool", value="true")
        etree.SubElement(OUT21, "Parameter", name="Coordinates_Bottom_Particle_4", type="bool", value="true")
        etree.SubElement(OUT21, "Parameter", name="Coordinates_Bottom_Particle_5", type="bool", value="true")
        

    def read_parameters(self, infile):

        with open(infile,'r') as i:
            lines = i.readlines()

        # Order of input parameters to be as the following:
        # Yield Stress
        self.par1 = float(lines[1].strip().split(" ")[0]) 
        # Hardening Strain Constant
        self.par2 = float(lines[2].strip().split(" ")[0]) 
        # Hardening Exponent
        self.par3 = float(lines[3].strip().split(" ")[0])
        # # Damage Parameter 1
        # self.par5 = 0.2
        # # Damage Parameter 2
        # self.par6 = 3.0
        # # Damage Parameter 3
        # self.par7 = 1.0

    def write_xml_file(self, outname="InputFileLongitudinal.xml"):

        root = etree.Element('ParameterList')

        # Create Discretization 
        self.__add_Discretization(root)

        # Create Materials 
        self.__add_Materials(root)

        # Create Blocks 
        self.__add_Blocks(root)

        # Create Boundary conditions
        self.__add_BC(root)

        # Create Compute Classes
        self.__add_compute_classes(root)

        # Create Solver
        self.__add_solver(root)

        # Create Output
        self.__add_output(root)

        with open(outname, 'wb') as f:
            # input_deck.write(f, pretty_print=True)
            f.write(etree.tostring(root, pretty_print=True))

inFile = sys.argv[1]
cal_test = calibration_test(grid_size, bar_thickness, extensometer_length, disp_rate, final_time, time_step, output_frequency1, output_frequency2)
cal_test.read_parameters(inFile)
cal_test.write_xml_file()
