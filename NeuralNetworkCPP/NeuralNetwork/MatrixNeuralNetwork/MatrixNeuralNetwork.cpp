//
//  NeuralNetwork.cpp
//  NeuralNetworkCPP
//
//  Created by MAC on 9/11/18.
//  Copyright © 2018 Tony Abou Zaidan. All rights reserved.
//

#include "MatrixNeuralNetwork.hpp"
using namespace std;
double MatrixNeuralNetwork::eta = 0.15;
double MatrixNeuralNetwork::alpha = 0.5;
double MatrixNeuralNetwork::m_recentAverageSmoothingFactor = 100.0;

MatrixNeuralNetwork::MatrixNeuralNetwork(int * _topology, int _topology_size, ActivationFunction* _act) : topology_size(_topology_size)
{
    this->topology = _topology;
    activationFunc = _act;
    weights = new Matrix<Weight>*[topology_size-1];
    neuron_outputs = new Matrix<double>*[topology_size];
    gradients = new Matrix<double>*[topology_size];
    
    for(int i=0; i<topology_size-1; i++)
    {
        weights[i] = new Matrix<Weight>(topology[i]+1, topology[i+1], true);
        neuron_outputs[i] = new Matrix<double>(1, topology[i] + 1);
        gradients[i] = new Matrix<double>(1, topology[i] + 1);
        neuron_outputs[i]->getMatrix()[0][neuron_outputs[i]->getColumns() -1] = 1;
    }
    
    neuron_outputs[topology_size-1] = new Matrix<double>(1, topology[topology_size-1], false);
    gradients[topology_size-1] = new Matrix<double>(1, topology[topology_size-1], false);
}

MatrixNeuralNetwork::MatrixNeuralNetwork(Matrix<Weight>*** w, int * _topology, int _topology_size, ActivationFunction* _act) : topology_size(_topology_size)
{
    this->topology = _topology;
    activationFunc = _act;
    neuron_outputs = new Matrix<double>*[topology_size];
    gradients = new Matrix<double>*[topology_size];
    for(int i=0; i<topology_size-1; i++)
    {
        neuron_outputs[i] = new Matrix<double>(1, topology[i] + 1, false);
        neuron_outputs[i]->getMatrix()[0][neuron_outputs[i]->getColumns() -1] = 1;
        gradients[i] = new Matrix<double>(1, topology[i] + 1);
    }
    
    this->weights = *w;
    
    neuron_outputs[topology_size-1] = new Matrix<double>(1, topology[topology_size-1], false);
    gradients[topology_size-1] = new Matrix<double>(1, topology[topology_size-1], false);
}

MatrixNeuralNetwork::~MatrixNeuralNetwork()
{
    delete[] topology;
    for(int i=0; i<topology_size-1; i++)
    {
        delete weights[i];
        delete neuron_outputs[i];
    }
    delete neuron_outputs[topology_size-1];
    delete[] weights;
    delete[] neuron_outputs;
    delete activationFunc;
}

Matrix<Weight> ** MatrixNeuralNetwork::getWeights()
{
    return weights;
}

Matrix<double> ** MatrixNeuralNetwork::getA()
{
    return neuron_outputs;
}

int* MatrixNeuralNetwork::getTopology()
{
    return topology;
}

int MatrixNeuralNetwork::getInputSize()
{
    return topology[0];
}

int MatrixNeuralNetwork::getTopologySize()
{
    return topology_size;
}

void MatrixNeuralNetwork::getResults(double** outputs)
{
    *outputs = new double[getResultsSize()];
    for(int i=0; i<getResultsSize(); i++)
        *outputs[i] = neuron_outputs[topology_size-1]->getMatrix()[0][i];
}

int MatrixNeuralNetwork::getResultsSize()
{
    return topology[topology_size-1];
}

void MatrixNeuralNetwork::feedForward(double *inputs)
{
    for(u_long i=neuron_outputs[0]->getColumnStart(); i<neuron_outputs[0]->getColumns() - 1 + neuron_outputs[0]->getColumnStart(); i++)
        neuron_outputs[0]->getMatrix()[0][i] = inputs[i];
    
    Matrix<double> * temp = nullptr;
    
    u_long c_e;
    for(int i=0; i<topology_size-1; i++)
    {
        temp = &(*neuron_outputs[i] * *weights[i]);
        c_e = temp->getColumns() + temp->getColumnStart();
        
        for(u_long k=temp->getColumnStart(); k<c_e; k++)
            neuron_outputs[i+1]->getMatrix()[0][k] = activationFunc->transferFunction(temp->getMatrix()[0][k]);
        
        delete temp;
    }
}

void MatrixNeuralNetwork::backProp(double* targets)
{
    double m_error = 0.0;
    double _temp;
    double* results = nullptr;
    getResults(&results);
    for(u_long i=0; i<topology[topology_size-1]; i++)
    {
        _temp = targets[i] - results[i];
        m_error = _temp * _temp;
    }
    delete[] results;
    m_error = m_error/topology[topology_size-1];
    m_error = std::sqrt(m_error);
    
    m_recentAverageError = (m_recentAverageError * m_recentAverageSmoothingFactor + m_error) / (m_recentAverageSmoothingFactor + 1.0);
    double output_val;
    for(u_long i=0; i<topology[topology_size-1]; i++)
    {
        output_val = neuron_outputs[topology_size-1]->getMatrix()[0][i];
        gradients[topology_size-1]->getMatrix()[0][i] = (targets[i] - output_val) * activationFunc->transferFunctionDerivative(output_val);
    }
    
    Matrix<double> * temp;
    u_long c_e;
    Matrix<Weight> * wt;
    Matrix<double> * gradient;
    
    
    for(u_long i = topology_size-1; i > 0; i--)
    {
        wt = &(weights[i-1]->transp());
        gradient = gradients[i];
        
        if(i != topology_size-1)
            gradient = &gradient->Extract(0, gradient->getRows(), 0, gradient->getColumns()-1); //Removing bias;
        
        temp = &(*gradient * *wt);
        
        c_e = temp->getColumns() + temp->getColumnStart();
        
        for(u_long k=temp->getColumnStart(); k<c_e; k++)
            gradients[i-1]->getMatrix()[0][k] = temp->getMatrix()[0][k] *
            activationFunc->transferFunctionDerivative(neuron_outputs[i-1]->getMatrix()[0][k]);
        
        delete temp;
        delete wt;
    }
    
    
    Matrix<Weight> * w_temp;
    u_long e_r;
    u_long e_c;
    for(int i = topology_size-2; i>=0; --i)
    {
        w_temp = weights[i];
        e_r = w_temp->getRowStart() + w_temp->getRows();
        e_c = w_temp->getColumnStart() + w_temp->getColumns();
        
        for(u_long j=w_temp->getRowStart(); j<e_r; j++)
        {
            for(u_long k = w_temp->getColumnStart(); k<e_c; k++)
            {
                double oldDeltaWeight = w_temp->getMatrix()[j][k].delta_weight;
                double newDeltaWeight = eta * neuron_outputs[i]->getMatrix()[0][j] * gradients[i+1]->getMatrix()[0][k] + alpha * oldDeltaWeight;
                w_temp->getMatrix()[j][k].delta_weight = newDeltaWeight;
                w_temp->getMatrix()[j][k].weight += newDeltaWeight;
            }
        }
    }
}

