#include "CameraCalibratorModule.hpp"

using namespace cv;

CameraCalibratorModule::CameraCalibratorModule(){
    
}

CameraCalibratorModule::~CameraCalibratorModule(){
    
}

void CameraCalibratorModule::ProcessFrame(cv::InputArray in, cv::OutputArray out){
    if(IsEnabled()){
        cv::Mat latestStep;
        in.copyTo(latestStep);
        
        



    
		latestStep.copyTo(out);
    }
    else{
        in.copyTo(out);
    }
}

void CameraCalibratorModule::DrawGUI(){
    if(showGUI){
        ImGui::Begin("Face Detector", &showGUI, ImGuiWindowFlags_AlwaysAutoResize);
        
		ImGui::Checkbox("Enabled", &enabled);
		ImGui::Separator();
        if(!enabled) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.2); //Push disabled style
		//Begin main content



        //End main content
        if(!enabled) ImGui::PopStyleVar(); //Pop disabled style
        
		ImGui::End();
    }
}