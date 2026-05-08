#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <chrono> 
#include <cstdlib>
#include <ctime>   

using namespace cv;
using namespace std;


enum Move { ROCK, PAPER, SCISSORS, UNKNOWN };


string moveToString(Move m) {
    if (m == ROCK) return "Rock";
    if (m == PAPER) return "Paper";
    if (m == SCISSORS) return "Scissors";
    return "Unknown";
}

int main() {
    
    srand(time(0));

    VideoCapture cap(0);
    if (!cap.isOpened()) {
        cerr << "Error: Cannot open the webcam. Check Mac Privacy settings!" << endl;
        return -1;
    }

    Mat frame, roi, hsv, mask;
    Rect roi_rect(50, 50, 400, 400);

   
    bool isPlaying = false;
    auto startTime = chrono::steady_clock::now();
    string gameStateText = "Press SPACE to start!";
    string resultText = "";
    string compMoveText = "";
    Move computerMove = UNKNOWN;
    Move playerMove = UNKNOWN;

    cout << "Press 'SPACE' to start a round." << endl;
    cout << "Press 'ESC' to exit the application." << endl;

    while (true) {
        cap >> frame;
        if (frame.empty()) break;

        flip(frame, frame, 1);
        
        
        rectangle(frame, roi_rect, Scalar(0, 255, 0), 2);
        roi = frame(roi_rect);

        cvtColor(roi, hsv, COLOR_BGR2HSV);
        inRange(hsv, Scalar(0, 40, 60), Scalar(20, 150, 255), mask);

        GaussianBlur(mask, mask, Size(5, 5), 0);
        dilate(mask, mask, Mat(), Point(-1, -1), 2);
        erode(mask, mask, Mat(), Point(-1, -1), 2);

        vector<vector<Point>> contours;
        findContours(mask, contours, RETR_TREE, CHAIN_APPROX_SIMPLE);

        int finger_gaps = -1; 
        bool handDetected = false;

        if (!contours.empty()) {
            size_t max_idx = 0;
            double max_area = 0;
            for (size_t i = 0; i < contours.size(); i++) {
                double area = contourArea(contours[i]);
                if (area > max_area) {
                    max_area = area;
                    max_idx = i;
                }
            }

            if (max_area > 5000) {
                handDetected = true;
                vector<Point> max_contour = contours[max_idx];
                vector<int> hull_indices;
                vector<Point> hull_points;
                
                convexHull(max_contour, hull_indices, false, false);
                convexHull(max_contour, hull_points, true, true);

                drawContours(roi, vector<vector<Point>>{hull_points}, -1, Scalar(0, 0, 255), 2);

                if (hull_indices.size() > 3) {
                    vector<Vec4i> defects;
                    convexityDefects(max_contour, hull_indices, defects);

                    finger_gaps = 0; 

                    for (const auto& defect : defects) {
                        Point ptStart = max_contour[defect[0]];
                        Point ptEnd = max_contour[defect[1]];
                        Point ptFar = max_contour[defect[2]];
                        float depth = defect[3] / 256.0f;

                        if (depth > 20) { 
                            double a = norm(ptEnd - ptStart);
                            double b = norm(ptFar - ptStart);
                            double c = norm(ptEnd - ptFar);
                            double angle = acos((b * b + c * c - a * a) / (2 * b * c)) * 180 / CV_PI;

                            if (angle <= 90) { 
                                finger_gaps++;
                                circle(roi, ptFar, 5, Scalar(255, 0, 0), -1);
                            }
                        }
                    }
                }
            }
        }

        
        if (!isPlaying) {
            
            putText(frame, gameStateText, Point(50, 40), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 255), 2);
            if (resultText != "") {
                putText(frame, resultText, Point(50, 480), FONT_HERSHEY_SIMPLEX, 1.5, Scalar(0, 255, 0), 3);
                putText(frame, "Comp chose: " + compMoveText, Point(50, 530), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 2);
            }
        } else {
            
            auto currentTime = chrono::steady_clock::now();
            double elapsedSeconds = chrono::duration_cast<chrono::milliseconds>(currentTime - startTime).count() / 1000.0;

            if (elapsedSeconds < 1.0) {
                gameStateText = "3...";
            } else if (elapsedSeconds < 2.0) {
                gameStateText = "2...";
            } else if (elapsedSeconds < 3.0) {
                gameStateText = "1...";
            } else if (elapsedSeconds < 3.5) {
                gameStateText = "SHOOT!";
                
                
                if (playerMove == UNKNOWN) {
                   
                    int randomChoice = rand() % 3;
                    if (randomChoice == 0) computerMove = ROCK;
                    else if (randomChoice == 1) computerMove = PAPER;
                    else computerMove = SCISSORS;

                    compMoveText = moveToString(computerMove);

                    
                    if (!handDetected) {
                        playerMove = UNKNOWN;
                    } else if (finger_gaps == 0) {
                        playerMove = ROCK;
                    } else if (finger_gaps == 1 || finger_gaps == 2) {
                        playerMove = SCISSORS; 
                    } else if (finger_gaps >= 3) {
                        playerMove = PAPER; 
                    }

                    
                    if (playerMove == UNKNOWN) {
                        resultText = "No hand detected!";
                    } else if (playerMove == computerMove) {
                        resultText = "It's a TIE!";
                    } else if ((playerMove == ROCK && computerMove == SCISSORS) ||
                               (playerMove == PAPER && computerMove == ROCK) ||
                               (playerMove == SCISSORS && computerMove == PAPER)) {
                        resultText = "YOU WIN!";
                    } else {
                        resultText = "COMPUTER WINS!";
                    }
                }
            } else {
                
                isPlaying = false;
                gameStateText = "Press SPACE to play again";
            }

            
            putText(frame, gameStateText, Point(50, 40), FONT_HERSHEY_SIMPLEX, 1.5, Scalar(0, 0, 255), 3);
            
            
            string currentGesture = "Reading: ";
            if (finger_gaps == 0) currentGesture += "Rock";
            else if (finger_gaps == 1 || finger_gaps == 2) currentGesture += "Scissors";
            else if (finger_gaps >= 3) currentGesture += "Paper";
            else currentGesture += "None";
            
            putText(frame, currentGesture, Point(50, 80), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(255, 100, 100), 2);
        }

        imshow("Rock Paper Scissors", frame);
        imshow("Skin Mask", mask);

        char key = (char)waitKey(30);
        if (key == 27) break; 
        if (key == ' ' && !isPlaying) { 
            isPlaying = true;
            startTime = chrono::steady_clock::now();
            playerMove = UNKNOWN;
            computerMove = UNKNOWN;
            resultText = "";
            compMoveText = "";
        }
    }

    cap.release();
    destroyAllWindows();
    return 0;
}
