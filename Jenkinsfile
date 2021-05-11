@Library('ecdc-pipeline')
import ecdcpipeline.ContainerBuildNode
import ecdcpipeline.PipelineBuilder

cppcheck_os = "debian10"

// Set number of old builds to keep.
properties([[
  $class: 'BuildDiscarderProperty',
  strategy: [
    $class: 'LogRotator',
    artifactDaysToKeepStr: '',
    artifactNumToKeepStr: '10',
    daysToKeepStr: '',
    numToKeepStr: ''
  ]
]]);

container_build_nodes = [
  'centos7': ContainerBuildNode.getDefaultContainerBuildNode('centos7-gcc8'),
  'debian10': ContainerBuildNode.getDefaultContainerBuildNode('debian10')
]

cmake_flags = [
  'centos7': '-DCMAKE_BUILD_TYPE=Release',
  'debian10': '-DCMAKE_BUILD_TYPE=Debug'
]

def failure_function(exception_obj, failureMessage) {
  def toEmails = [[$class: 'DevelopersRecipientProvider']]
  emailext body: '${DEFAULT_CONTENT}\n\"' + failureMessage + '\"\n\nCheck console output at $BUILD_URL to view the results.',
    recipientProviders: toEmails,
    subject: '${DEFAULT_SUBJECT}'
    
  throw exception_obj
}

pipeline_builder = new PipelineBuilder(this, container_build_nodes, "/mnt/data:/home/jenkins/refdata")

builders = pipeline_builder.createBuilders { container ->

  pipeline_builder.stage("${container.key}: checkout") {
    dir(pipeline_builder.project) {
      scm_vars = checkout scm
    }
    // Copy source code to container.
    container.copyTo(pipeline_builder.project, pipeline_builder.project)
  }  // stage

  if (container.key == cppcheck_os) {
    pipeline_builder.stage("${container.key}: cppcheck") {
      container.sh """
        cd ${pipeline_builder.project}
        cppcheck --enable=all --inconclusive --template="{file},{line},{severity},{id},{message}" ./ 2> cppcheck.txt
      """
      container.copyFrom(pipeline_builder.project, ".")
      sh "mv -f ./${pipeline_builder.project}/* ./"
      step([
        $class: 'WarningsPublisher',
        parserConfigurations: [[parserName: 'Cppcheck Parser', pattern: "cppcheck.txt"]]
      ])
    }  // stage
  } else {  // Not the Cppcheck OS
      pipeline_builder.stage("${container.key}: configure") {
      def local_cmake_flags = cmake_flags[container.key]
      container.sh """
        mkdir ${pipeline_builder.project}/build
        cd ${pipeline_builder.project}/build
        cmake --version
        cmake ${local_cmake_flags} -DCONAN=AUTO -DCAEN_ROOT=/home/jenkins/${pipeline_builder.project}/libcaen ..
      """
    }  // stage

    pipeline_builder.stage("${container.key}: build") {
      container.sh """
        cd ${pipeline_builder.project}/build
        make --version
        make -j8
      """
    }  // stage
  }  // if/else

}  // createBuilders

node('docker') {
  // Delete workspace when build is done
  cleanWs()

  dir("${pipeline_builder.project}_code") {
    stage('Checkout') {
      try {
        scm_vars = checkout scm
      } catch (e) {
        failure_function(e, 'Checkout failed')
      }
    }

    stage("Static analysis") {
      try {
        sh "find . -name '*TestData.h' > exclude_cloc"
        sh "cloc --exclude-list-file=exclude_cloc --by-file --xml --out=cloc.xml ."
        sh "pwd"
        sh "find .. -name cloc2sloccount.xsl"
        sh "xsltproc jenkins/cloc2sloccount.xsl cloc.xml > sloccount.sc"
        sloccountPublish encoding: '', pattern: ''
      } catch (e) {
        failure_function(e, 'Static analysis failed')
      }
    }
  }

  try {
    timeout(time: 1, unit: 'HOURS') {
      parallel builders
    }
  } catch (e) {
    failure_function(e, 'Job failed')
  }
}
