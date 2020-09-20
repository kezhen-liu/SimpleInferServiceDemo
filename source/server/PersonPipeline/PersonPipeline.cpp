#include <functional>
#include <iostream>
#include <fstream>
#include <random>
#include <memory>
#include <chrono>
#include <algorithm>
#include <iterator>
#include <map>
#include <string>
#include <vector>
#include <set>
#include <unordered_map>

#include <boost/exception/all.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <inference_engine.hpp>
#include <server/PersonPipeline/slog.hpp>
#include <server/PersonPipeline/ocv_common.hpp>

#include <server/PersonPipeline/PersonPipeline.hpp>

using namespace InferenceEngine;

namespace SISD{
// -------------------------Generic routines for detection networks-------------------------------------------------

struct BaseDetection {
    ExecutableNetwork net;
    InferRequest request;
    std::string commandLineFlag;
    std::string topoName;
    Blob::Ptr inputBlob;
    std::string inputName;
    std::string outputName;

    BaseDetection(const std::string &commandLineFlag, const std::string &topoName)
            : commandLineFlag(commandLineFlag), topoName(topoName) {}

    ExecutableNetwork * operator ->() {
        return &net;
    }
    virtual CNNNetwork read(const Core& ie)  = 0;

    virtual void setRoiBlob(const Blob::Ptr &roiBlob) {
        if (!enabled())
            return;
        if (!request)
            request = net.CreateInferRequest();

        request.SetBlob(inputName, roiBlob);
    }

    virtual void enqueue(const cv::Mat &person) {
        if (!enabled())
            return;
        if (!request)
            request = net.CreateInferRequest();

        inputBlob = request.GetBlob(inputName);
        matU8ToBlob<uint8_t>(person, inputBlob);
    }

    virtual void submitRequest() {
        if (!enabled() || !request) return;
        request.StartAsync();
    }

    virtual void wait() {
        if (!enabled()|| !request) return;
        request.Wait(IInferRequest::WaitMode::RESULT_READY);
    }
    mutable bool enablingChecked = false;
    mutable bool _enabled = false;

    bool enabled() const  {
        if (!enablingChecked) {
            _enabled = !commandLineFlag.empty();
            if (!_enabled) {
                slog::info << topoName << " detection DISABLED" << slog::endl;
            }
            enablingChecked = true;
        }
        return _enabled;
    }

    void printPerformanceCounts(std::string fullDeviceName) const {
        ::printPerformanceCounts(request, std::cout, fullDeviceName);
    }
};

struct PersonDetection : BaseDetection{
    int maxProposalCount;
    int objectSize;
    float width = 0.0f;
    float height = 0.0f;
    bool resultsFetched = false;

    struct Result {
        int label;
        float confidence;
        cv::Rect location;
    };

    std::vector<Result> results;

    void submitRequest() override {
        resultsFetched = false;
        results.clear();
        BaseDetection::submitRequest();
    }

    void setRoiBlob(const Blob::Ptr &frameBlob) override {
        height = static_cast<float>(frameBlob->getTensorDesc().getDims()[2]);
        width = static_cast<float>(frameBlob->getTensorDesc().getDims()[3]);
        BaseDetection::setRoiBlob(frameBlob);
    }

    void enqueue(const cv::Mat &frame) override {
        height = static_cast<float>(frame.rows);
        width = static_cast<float>(frame.cols);
        BaseDetection::enqueue(frame);
    }

    PersonDetection() : BaseDetection("person-vehicle-bike-detection-crossroad-0078.xml", "Person Detection"), maxProposalCount(0), objectSize(0) {}
    CNNNetwork read(const Core& ie) override {
        slog::info << "Loading network files for PersonDetection" << slog::endl;
        /** Read network model **/
        auto network = ie.ReadNetwork("person-vehicle-bike-detection-crossroad-0078.xml");
        /** Set batch size to 1 **/
        slog::info << "Batch size is forced to  1" << slog::endl;
        network.setBatchSize(1);
        // -----------------------------------------------------------------------------------------------------

        /** SSD-based network should have one input and one output **/
        // ---------------------------Check inputs ------------------------------------------------------
        slog::info << "Checking Person Detection inputs" << slog::endl;
        InputsDataMap inputInfo(network.getInputsInfo());
        if (inputInfo.size() != 1) {
            throw std::logic_error("Person Detection network should have only one input");
        }
        InputInfo::Ptr& inputInfoFirst = inputInfo.begin()->second;
        inputInfoFirst->setPrecision(Precision::U8);


        inputInfoFirst->getInputData()->setLayout(Layout::NCHW);
        inputName = inputInfo.begin()->first;
        // -----------------------------------------------------------------------------------------------------

        // ---------------------------Check outputs ------------------------------------------------------
        slog::info << "Checking Person Detection outputs" << slog::endl;
        OutputsDataMap outputInfo(network.getOutputsInfo());
        if (outputInfo.size() != 1) {
            throw std::logic_error("Person Detection network should have only one output");
        }
        DataPtr& _output = outputInfo.begin()->second;
        const SizeVector outputDims = _output->getTensorDesc().getDims();
        outputName = outputInfo.begin()->first;
        maxProposalCount = outputDims[2];
        objectSize = outputDims[3];
        if (objectSize != 7) {
            throw std::logic_error("Output should have 7 as a last dimension");
        }
        if (outputDims.size() != 4) {
            throw std::logic_error("Incorrect output dimensions for SSD");
        }
        _output->setPrecision(Precision::FP32);
        _output->setLayout(Layout::NCHW);

        slog::info << "Loading Person Detection model to CPU" << slog::endl;
        return network;
    }

    void fetchResults() {
        if (!enabled()) return;
        results.clear();
        if (resultsFetched) return;
        resultsFetched = true;
        LockedMemory<const void> outputMapped = as<MemoryBlob>(request.GetBlob(outputName))->rmap();
        const float *detections = outputMapped.as<float *>();
        // pretty much regular SSD post-processing
        for (int i = 0; i < maxProposalCount; i++) {
            float image_id = detections[i * objectSize + 0];  // in case of batch
            if (image_id < 0) {  // indicates end of detections
                break;
            }

            Result r;
            r.label = static_cast<int>(detections[i * objectSize + 1]);
            r.confidence = detections[i * objectSize + 2];

            r.location.x = static_cast<int>(detections[i * objectSize + 3] * width);
            r.location.y = static_cast<int>(detections[i * objectSize + 4] * height);
            r.location.width = static_cast<int>(detections[i * objectSize + 5] * width - r.location.x);
            r.location.height = static_cast<int>(detections[i * objectSize + 6] * height - r.location.y);

            std::cout << "[" << i << "," << r.label << "] element, prob = " << r.confidence <<
                        "    (" << r.location.x << "," << r.location.y << ")-(" << r.location.width << ","
                        << r.location.height << ")"
                        << ((r.confidence > 0.72) ? " WILL BE RENDERED!" : "") << std::endl;

            if (r.confidence <= 0.72) {
                continue;
            }
            results.push_back(r);
        }
    }
};

struct PersonAttribsDetection : BaseDetection {
    std::string outputNameForAttributes;
    std::string outputNameForTopColorPoint;
    std::string outputNameForBottomColorPoint;


    PersonAttribsDetection() : BaseDetection("person-attributes-recognition-crossroad-0230.xml", "Person Attributes Recognition") {}

    struct AttributesAndColorPoints{
        std::vector<std::string> attributes_strings;
        std::vector<bool> attributes_indicators;
        cv::Point2f top_color_point;
        cv::Point2f bottom_color_point;
        cv::Vec3b top_color;
        cv::Vec3b bottom_color;
    };

    static cv::Vec3b GetAvgColor(const cv::Mat& image) {
        int clusterCount = 5;
        cv::Mat labels;
        cv::Mat centers;
        cv::Mat image32f;
        image.convertTo(image32f, CV_32F);
        image32f = image32f.reshape(1, image32f.rows*image32f.cols);
        clusterCount = std::min(clusterCount, image32f.rows);
        cv::kmeans(image32f, clusterCount, labels, cv::TermCriteria(cv::TermCriteria::EPS+cv::TermCriteria::MAX_ITER, 10, 1.0),
                    10, cv::KMEANS_RANDOM_CENTERS, centers);
        centers.convertTo(centers, CV_8U);
        centers = centers.reshape(0, clusterCount);
        std::vector<int> freq(clusterCount);

        for (int i = 0; i < labels.rows * labels.cols; ++i) {
            freq[labels.at<int>(i)]++;
        }

        auto freqArgmax = std::max_element(freq.begin(), freq.end()) - freq.begin();

        return centers.at<cv::Vec3b>(freqArgmax);
    }

    AttributesAndColorPoints GetPersonAttributes() {
        static const char *const attributeStrings[] = {
                "is male", "has_bag", "has_backpack" , "has hat", "has longsleeves", "has longpants", "has longhair", "has coat_jacket"
        };

        Blob::Ptr attribsBlob = request.GetBlob(outputNameForAttributes);
        Blob::Ptr topColorPointBlob = request.GetBlob(outputNameForTopColorPoint);
        Blob::Ptr bottomColorPointBlob = request.GetBlob(outputNameForBottomColorPoint);
        size_t numOfAttrChannels = attribsBlob->getTensorDesc().getDims().at(1);
        size_t numOfTCPointChannels = topColorPointBlob->getTensorDesc().getDims().at(1);
        size_t numOfBCPointChannels = bottomColorPointBlob->getTensorDesc().getDims().at(1);

        if (numOfAttrChannels != arraySize(attributeStrings)) {
            throw std::logic_error("Output size (" + std::to_string(numOfAttrChannels) + ") of the "
                                   "Person Attributes Recognition network is not equal to expected "
                                   "number of attributes (" + std::to_string(arraySize(attributeStrings)) + ")");
        }
        if (numOfTCPointChannels != 2) {
            throw std::logic_error("Output size (" + std::to_string(numOfTCPointChannels) + ") of the "
                                   "Person Attributes Recognition network is not equal to point coordinates(2)");
        }
        if (numOfBCPointChannels != 2) {
            throw std::logic_error("Output size (" + std::to_string(numOfBCPointChannels) + ") of the "
                                   "Person Attributes Recognition network is not equal to point coordinates (2)");
        }

        LockedMemory<const void> attribsBlobMapped = as<MemoryBlob>(attribsBlob)->rmap();
        auto outputAttrValues = attribsBlobMapped.as<float*>();
        LockedMemory<const void> topColorPointBlobMapped = as<MemoryBlob>(topColorPointBlob)->rmap();
        auto outputTCPointValues = topColorPointBlobMapped.as<float*>();
        LockedMemory<const void> bottomColorPointBlobMapped = as<MemoryBlob>(bottomColorPointBlob)->rmap();
        auto outputBCPointValues = bottomColorPointBlobMapped.as<float*>();

        AttributesAndColorPoints returnValue;

        returnValue.top_color_point.x = outputTCPointValues[0];
        returnValue.top_color_point.y = outputTCPointValues[1];

        returnValue.bottom_color_point.x = outputBCPointValues[0];
        returnValue.bottom_color_point.y = outputBCPointValues[1];

        for (size_t i = 0; i < arraySize(attributeStrings); i++) {
            returnValue.attributes_strings.push_back(attributeStrings[i]);
            returnValue.attributes_indicators.push_back(outputAttrValues[i] > 0.5);
        }

        return returnValue;
    }

    CNNNetwork read(const Core& ie) override {
        slog::info << "Loading network files for PersonAttribs" << slog::endl;
        /** Read network model **/
        auto network = ie.ReadNetwork("person-attributes-recognition-crossroad-0230.xml");
        /** Extract model name and load it's weights **/
        network.setBatchSize(1);
        slog::info << "Batch size is forced to 1 for Person Attribs" << slog::endl;
        // -----------------------------------------------------------------------------------------------------

        /** Person Attribs network should have one input two outputs **/
        // ---------------------------Check inputs ------------------------------------------------------
        slog::info << "Checking PersonAttribs inputs" << slog::endl;
        InputsDataMap inputInfo(network.getInputsInfo());
        if (inputInfo.size() != 1) {
            throw std::logic_error("Person Attribs topology should have only one input");
        }
        InputInfo::Ptr& inputInfoFirst = inputInfo.begin()->second;
        inputInfoFirst->setPrecision(Precision::U8);

        inputInfoFirst->getInputData()->setLayout(Layout::NCHW);
        inputName = inputInfo.begin()->first;
        // -----------------------------------------------------------------------------------------------------

        // ---------------------------Check outputs ------------------------------------------------------
        slog::info << "Checking Person Attribs outputs" << slog::endl;
        OutputsDataMap outputInfo(network.getOutputsInfo());
        if (outputInfo.size() != 3) {
             throw std::logic_error("Person Attribs Network expects networks having one output");
        }
        auto it = outputInfo.begin();
        outputNameForAttributes = (it++)->second->getName();  // attribute probabilities
        outputNameForTopColorPoint = (it++)->second->getName();  // top color location
        outputNameForBottomColorPoint = (it++)->second->getName();  // bottom color location
        slog::info << "Loading Person Attributes Recognition model to CPU" << slog::endl;
        _enabled = true;
        return network;
    }
};

struct Load {
    BaseDetection& detector;
    explicit Load(BaseDetection& detector) : detector(detector) { }

    void into(Core & ie, const std::string & deviceName) const {
        if (detector.enabled()) {
            detector.net = ie.LoadNetwork(detector.read(ie), deviceName);
        }
    }
};


class PersonPipeline::Impl{
public:
    struct ROI{
        unsigned x;
        unsigned y;
        unsigned w;
        unsigned h;
        std::string attrib;
    };

    struct Result{
        using ROIVec = std::vector<ROI>;
        ROIVec rois;
        std::string imageName;
    };

    using ResultVec = std::vector<Result>;

    Impl();

    ~Impl();

    bool init();

    std::string run(const char* input, std::size_t size, const std::string& imageName);

private:
    std::string constructJsonMessage(const ResultVec& results) const;

    PersonDetection m_personDetection;
    PersonAttribsDetection m_personAttribs;
};

PersonPipeline::Impl::Impl(){

}

PersonPipeline::Impl::~Impl(){

}

bool PersonPipeline::Impl::init(){
    try {
        std::cout << "InferenceEngine: " << GetInferenceEngineVersion() << std::endl;

        // --------------------------- 1. Load inference engine -------------------------------------
        Core ie;

        std::set<std::string> loadedDevices;

        std::vector<std::string> deviceNames = {
                "CPU",
                "CPU",
                "CPU"
        };

        for (auto && flag : deviceNames) {
            if (flag.empty())
                continue;

            auto i = loadedDevices.find(flag);
            if (i != loadedDevices.end()) {
                continue;
            }
            slog::info << "Loading device " << flag << slog::endl;

            /** Printing device version **/
            std::cout << ie.GetVersions(flag) << std::endl;

            loadedDevices.insert(flag);
        }

        // --------------------------- 2. Read IR models and load them to devices ------------------------------
        Load(m_personDetection).into(ie, "CPU");
        Load(m_personAttribs).into(ie, "CPU");
    }
    catch (const std::exception& error) {
        std::cerr << "[ ERROR ] " << error.what() << std::endl;
        return false;
    }
    catch (...) {
        std::cerr << "[ ERROR ] Unknown/internal exception happened." << std::endl;
        return false;
    }
    return true;
}

std::string PersonPipeline::Impl::run(const char* input, std::size_t size, const std::string& imageName){
    std::string jsonOut = "";
    try{
        ::cv::Mat rawData( 1, size, CV_8UC1, (void*)input );

        ::cv::Mat frame = cv::imdecode(rawData, ::cv::IMREAD_COLOR);

        const size_t width  = frame.size().width;
        const size_t height = frame.size().height;

        // --------------------------- 3. Do inference ---------------------------------------------------------
        Blob::Ptr frameBlob;  // Blob to be used to keep processed frame data
        ROI cropRoi;  // cropped image coordinates
        Blob::Ptr roiBlob;  // This blob contains data from cropped image (vehicle or license plate)
        cv::Mat person;  // Mat object containing person data cropped by openCV

        /** Start inference & calc performance **/
        typedef std::chrono::duration<double, std::ratio<1, 1000>> ms;
        auto total_t0 = std::chrono::high_resolution_clock::now();
        slog::info << "Start inference " << slog::endl;

        do {
            m_personDetection.enqueue(frame);
            // --------------------------- Run Person detection inference --------------------------------------
            auto t0 = std::chrono::high_resolution_clock::now();
            m_personDetection.submitRequest();
            m_personDetection.wait();
            auto t1 = std::chrono::high_resolution_clock::now();
            ms detection = std::chrono::duration_cast<ms>(t1 - t0);
            // parse inference results internally (e.g. apply a threshold, etc)
            m_personDetection.fetchResults();
            // -------------------------------------------------------------------------------------------------

            // --------------------------- Process the results down to the pipeline ----------------------------
            ms personAttribsNetworkTime(0), personReIdNetworktime(0);
            int personAttribsInferred = 0,  personReIdInferred = 0;
            Result res;
            for (auto && result : m_personDetection.results) {
                if (result.label == 1) {  // person
                    auto clippedRect = result.location & cv::Rect(0, 0, width, height);
                    person = frame(clippedRect);

                    PersonAttribsDetection::AttributesAndColorPoints resPersAttrAndColor;
                    std::string resPersReid = "";
                    cv::Point top_color_p;
                    cv::Point bottom_color_p;


                    // --------------------------- Run Person Attributes Recognition -----------------------
                    m_personAttribs.enqueue(person);

                    t0 = std::chrono::high_resolution_clock::now();
                    m_personAttribs.submitRequest();
                    m_personAttribs.wait();
                    t1 = std::chrono::high_resolution_clock::now();
                    personAttribsNetworkTime += std::chrono::duration_cast<ms>(t1 - t0);
                    personAttribsInferred++;
                    // --------------------------- Process outputs -----------------------------------------

                    resPersAttrAndColor = m_personAttribs.GetPersonAttributes();

                    top_color_p.x = static_cast<int>(resPersAttrAndColor.top_color_point.x) * person.cols;
                    top_color_p.y = static_cast<int>(resPersAttrAndColor.top_color_point.y) * person.rows;

                    bottom_color_p.x = static_cast<int>(resPersAttrAndColor.bottom_color_point.x) * person.cols;
                    bottom_color_p.y = static_cast<int>(resPersAttrAndColor.bottom_color_point.y) * person.rows;


                    cv::Rect person_rect(0, 0, person.cols, person.rows);

                    // Define area around top color's location
                    cv::Rect tc_rect;
                    tc_rect.x = top_color_p.x - person.cols / 6;
                    tc_rect.y = top_color_p.y - person.rows / 10;
                    tc_rect.height = 2 * person.rows / 8;
                    tc_rect.width = 2 * person.cols / 6;

                    tc_rect = tc_rect & person_rect;

                    // Define area around bottom color's location
                    cv::Rect bc_rect;
                    bc_rect.x = bottom_color_p.x - person.cols / 6;
                    bc_rect.y = bottom_color_p.y - person.rows / 10;
                    bc_rect.height =  2 * person.rows / 8;
                    bc_rect.width = 2 * person.cols / 6;

                    bc_rect = bc_rect & person_rect;

                    resPersAttrAndColor.top_color = PersonAttribsDetection::GetAvgColor(person(tc_rect));
                    resPersAttrAndColor.bottom_color = PersonAttribsDetection::GetAvgColor(person(bc_rect));

                    // --------------------------- Process outputs -----------------------------------------
                    if (!resPersAttrAndColor.attributes_strings.empty()) {
                        std::string output_attribute_string;
                        for (size_t i = 0; i < resPersAttrAndColor.attributes_strings.size(); ++i)
                            if (resPersAttrAndColor.attributes_indicators[i])
                                output_attribute_string += resPersAttrAndColor.attributes_strings[i] + ",";
                        std::cout << "Person ROI: " << result.location.x << ", " << result.location.y << ". "
                            << result.location.width << ", " << result.location.height << std::endl;
                        std::cout << "Person Attributes results: " << output_attribute_string << std::endl;
                        std::cout << "Person top color: " << resPersAttrAndColor.top_color << std::endl;
                        std::cout << "Person bottom color: " << resPersAttrAndColor.bottom_color << std::endl;
                        ROI roi;
                        roi.x = result.location.x;
                        roi.y = result.location.y;
                        roi.w = result.location.width;
                        roi.h = result.location.height;
                        roi.attrib = output_attribute_string;
                        res.rois.push_back(roi);
                    }
                }
            }
            res.imageName = imageName;
            jsonOut = constructJsonMessage(ResultVec{res});
            std::cout << jsonOut << std::endl;

        } while(false);

        auto total_t1 = std::chrono::high_resolution_clock::now();
        ms total = std::chrono::duration_cast<ms>(total_t1 - total_t0);
        slog::info << "Total Inference time: " << total.count() << slog::endl;

    }
    catch (const std::exception& error) {
        std::cerr << "[ ERROR ] " << error.what() << std::endl;
    }
    catch (...) {
        std::cerr << "[ ERROR ] Unknown/internal exception happened." << std::endl;
    }
    return jsonOut;
}

std::string PersonPipeline::Impl::constructJsonMessage(const ResultVec& results) const{
    boost::property_tree::ptree jsonTree;

    for(const auto& frame: results){
        boost::property_tree::ptree frameNode;

        for(const auto& roi: frame.rois){
            boost::property_tree::ptree roiNode;
            roiNode.put("x", roi.x);
            roiNode.put("y", roi.y);
            roiNode.put("width", roi.w);
            roiNode.put("height", roi.h);
            roiNode.put("attributes", roi.attrib);

            frameNode.push_back(std::make_pair("", roiNode));
        }
        if(!frameNode.empty()){
            // jsonTree.add_child(frame.imageName, frameNode);
            jsonTree.push_back(std::make_pair(frame.imageName, frameNode));
        }
    }

    if(jsonTree.empty()){
        return "";
    }
    else{
        std::stringstream ss;
        boost::property_tree::json_parser::write_json(ss, jsonTree);
        return ss.str();
    }
}

PersonPipeline::PersonPipeline(){
    m_impl = std::unique_ptr<Impl>(new Impl);
}

PersonPipeline::~PersonPipeline(){
    
}

bool PersonPipeline::init(){
    return m_impl->init();
}

std::string PersonPipeline::run(const char* input, std::size_t size, const std::string& imageName){
    return m_impl->run(input, size, imageName);
}
}