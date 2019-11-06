import qbs
import "../../qbs/utility.qbs" as Utility

Utility {
    projectName: "parse_flaticon"
    property string organization: sourceDirectory
    cpp.defines: "organizationq=" + organization
}
