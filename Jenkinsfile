project = "jadaq"

cppcheck_os = "debian9"

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

images = [
    'centos7': [
        'name': 'screamingudder/centos7-build-node:4.6.0',
        'cmake': 'cmake3',
        'sh': '/usr/bin/scl enable devtoolset-6 -- /bin/bash -e',
        'cmake_flags': '-DCMAKE_BUILD_TYPE=Release'
    ],
    'debian9': [
        'name': 'screamingudder/debian9-build-node:3.4.0',
        'cmake': 'cmake3',
        'sh'  : 'bash -e',
        'cmake_flags': '-DCMAKE_BUILD_TYPE=Debug'
    ]
]

base_container_name = "${project}-${env.BRANCH_NAME}-${env.BUILD_NUMBER}"

def failure_function(exception_obj, failureMessage) {
    def toEmails = [[$class: 'DevelopersRecipientProvider']]
    emailext body: '${DEFAULT_CONTENT}\n\"' + failureMessage + '\"\n\nCheck console output at $BUILD_URL to view the results.',
            recipientProviders: toEmails,
            subject: '${DEFAULT_SUBJECT}'
    slackSend color: 'danger',
            message: "${project}-${env.BRANCH_NAME}: " + failureMessage
    throw exception_obj
}

def Object container_name(image_key) {
    return "${base_container_name}-${image_key}"
}

def Object get_container(image_key) {
    def image = docker.image(images[image_key]['name'])
    def container = image.run("\
        --name ${container_name(image_key)} \
        --tty \
        --env http_proxy=${env.http_proxy} \
        --env https_proxy=${env.https_proxy} \
        --env local_conan_server=${env.local_conan_server} \
        ")
    return container
}



def docker_copy_code(image_key) {
    def custom_sh = images[image_key]['sh']
    sh "docker cp ${project}_code ${container_name(image_key)}:/home/jenkins/${project}"
    sh """docker exec --user root ${container_name(image_key)} ${custom_sh} -c \"
                        chown -R jenkins.jenkins /home/jenkins/${project}
                        \""""
}

def docker_dependencies(image_key) {
    def conan_remote = "ess-dmsc-local"
    def custom_sh = images[image_key]['sh']
    sh """docker exec ${container_name(image_key)} ${custom_sh} -c \"
        mkdir ${project}/build
        cd ${project}/build
        conan remote add \
            --insert 0 \
            ${conan_remote} ${local_conan_server}
    \""""
}

def docker_cmake(image_key, xtra_flags) {
    cmake_exec = images[image_key]['cmake']
    def custom_sh = images[image_key]['sh']
    sh """docker exec ${container_name(image_key)} ${custom_sh} -c \"
        cd ${project}
        cd build
        ${cmake_exec} --version
        conan install .. --build=missing
        ${cmake_exec} -DCAEN_ROOT=/home/jenkins/jadaq/libcaen ${xtra_flags} -DCONAN=MANUAL ..
    \""""
}

def docker_build(image_key) {
    def custom_sh = images[image_key]['sh']
    sh """docker exec ${container_name(image_key)} ${custom_sh} -c \"
        cd ${project}/build
        make --version
        make
    \""""
}


def docker_cppcheck(image_key) {
    try {
        def custom_sh = images[image_key]['sh']
        def test_output = "cppcheck.txt"
        def cppcheck_script = """
                        cd ${project}
                        cppcheck --enable=all --inconclusive --template="{file},{line},{severity},{id},{message}" ./ 2> ${test_output}
                    """
        sh "docker exec ${container_name(image_key)} ${custom_sh} -c \"${cppcheck_script}\""
        sh "docker cp ${container_name(image_key)}:/home/jenkins/${project} ."
        sh "mv -f ./${project}/* ./"
    } catch (e) {
        failure_function(e, "Cppcheck step for (${container_name(image_key)}) failed")
    }
}


def get_pipeline(image_key)
{
    return {
        stage("${image_key}") {
            try {
                def container = get_container(image_key)

                docker_copy_code(image_key)
                if (image_key != cppcheck_os) {
                    docker_dependencies(image_key)
                    docker_cmake(image_key, images[image_key]['cmake_flags'])
                    docker_build(image_key)
                }

                if (image_key == cppcheck_os) {
                  docker_cppcheck(image_key)
                  step([$class: 'WarningsPublisher', parserConfigurations: [[parserName: 'Cppcheck Parser', pattern: "cppcheck.txt"]]])
                  docker_dependencies(image_key)
                  docker_cmake(image_key, images[image_key]['cmake_flags'])
                  docker_build(image_key)
                }
            } finally {
                sh "docker stop ${container_name(image_key)}"
                sh "docker rm -f ${container_name(image_key)}"
            }
        }
    }
}

node('docker') {
    // Delete workspace when build is done
    cleanWs()

    dir("${project}_code") {

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

    def builders = [:]

    for (x in images.keySet()) {
        def image_key = x
        builders[image_key] = get_pipeline(image_key)
    }

    try {
        parallel builders
    } catch (e) {
        failure_function(e, 'Job failed')
    }
}
