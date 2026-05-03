#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <chrono> // For the countdown timer
#include <cstdlib> // For random numbers
#include <ctime>   // For random seed

using namespace cv;
using namespace std;

// Enum to make our game logic easy to read
enum Move { ROCK, PAPER, SCISSORS, UNKNOWN };

// Helper function to turn the enum into text
string moveToString(Move m) {
    if (m == ROCK) return "Rock";
    if (m == PAPER) return "Paper";
    if (m == SCISSORS) return "Scissors";
    return "Unknown";
}

int main() {
    // Seed the random number generator for the computer's moves
    srand(time(0));

    VideoCapture cap(0);
    if (!cap.isOpened()) {
        cerr << "Error: Cannot open the webcam. Check Mac Privacy settings!" << endl;
        return -1;
    }

    Mat frame, roi, hsv, mask;
    Rect roi_rect(50, 50, 400, 400);

    // --- GAME STATE VARIABLES ---
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
        
        // Draw the ROI where the player needs to put their hand
        rectangle(frame, roi_rect, Scalar(0, 255, 0), 2);
        roi = frame(roi_rect);

        cvtColor(roi, hsv, COLOR_BGR2HSV);
        inRange(hsv, Scalar(0, 40, 60), Scalar(20, 150, 255), mask);

        GaussianBlur(mask, mask, Size(5, 5), 0);
        dilate(mask, mask, Mat(), Point(-1, -1), 2);
        erode(mask, mask, Mat(), Point(-1, -1), 2);

        vector<vector<Point>> contours;
        findContours(mask, contours, RETR_TREE, CHAIN_APPROX_SIMPLE);

        int finger_gaps = -1; // Default to -1 (meaning no hand/gesture detected)
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

                    finger_gaps = 0; // Hand detected, start counting gaps

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

        // --- GAME LOGIC ---
        if (!isPlaying) {
            // Waiting for player to press space
            putText(frame, gameStateText, Point(50, 40), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 255), 2);
            if (resultText != "") {
                putText(frame, resultText, Point(50, 480), FONT_HERSHEY_SIMPLEX, 1.5, Scalar(0, 255, 0), 3);
                putText(frame, "Comp chose: " + compMoveText, Point(50, 530), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 2);
            }
        } else {
            // Calculate time elapsed since spacebar was pressed
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
                
                // Read the gesture ONCE during this 0.5 second window
                if (playerMove == UNKNOWN) {
                    // Computer makes a random choice (0 = Rock, 1 = Paper, 2 = Scissors)
                    int randomChoice = rand() % 3;
                    if (randomChoice == 0) computerMove = ROCK;
                    else if (randomChoice == 1) computerMove = PAPER;
                    else computerMove = SCISSORS;

                    compMoveText = moveToString(computerMove);

                    // Determine Player Move based on gaps
                    if (!handDetected) {
                        playerMove = UNKNOWN;
                    } else if (finger_gaps == 0) {
                        playerMove = ROCK;
                    } else if (finger_gaps == 1 || finger_gaps == 2) {
                        playerMove = SCISSORS; // 1 or 2 gaps is usually a peace sign
                    } else if (finger_gaps >= 3) {
                        playerMove = PAPER; // 3 or 4 gaps is an open hand
                    }

                    // Determine Winner
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
                // Timer finished, reset state to show results
                isPlaying = false;
                gameStateText = "Press SPACE to play again";
            }

            // Display current countdown or SHOOT text
            putText(frame, gameStateText, Point(50, 40), FONT_HERSHEY_SIMPLEX, 1.5, Scalar(0, 0, 255), 3);
            
            // Show real-time gesture reading so the player knows what the camera sees
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
        if (key == 27) break; // ESC to quit
        if (key == ' ' && !isPlaying) { // SPACE to start
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